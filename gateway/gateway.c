/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <r9k/compiler_attrs.h>

#include "io/connection.h"
#include "io/socket.h"
#include "utils/log.h"
#include "ipc.h"
#include "config.h"

extern void client_start();

static int _init(int *p_listen, int *p_epfd)
{
        int listen_fd;
        int epfd;

        listen_fd = tcp_create_listener(PORT);

        if (listen_fd < 0) {
                log_error("listen socket create failed: %s\n", syserr);
                return -1;
        }

        epfd = epoll_create1(EPOLL_CLOEXEC);

        if (epfd < 0) {
                log_error("epoll fd create failed: %s\n", syserr);
                return -1;
        }

        struct epoll_event tev = {
                .events = EPOLLIN,
                .data.fd = listen_fd,
        };

        if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &tev) < 0) {
                log_error("epoll_ctl add listen_fd failed: %s\n", syserr);
                return -1;
        }

        *p_listen = listen_fd;
        *p_epfd   = epfd;

        log_info("listening on port: %d\n", PORT);

        return 0;
}

static void on_event_accept(int epfd, int listen_fd)
{
        int cli;
        struct host_sockaddr_in addr;
        struct connection *conn;

        while (true) {
                cli = tcp_accept(listen_fd, &addr);

                if (cli < 0) {
                        RETRY_IF_EINTR();

                        if (is_eagain())
                                break;
                }

                conn = connection_create(cli, &addr);

                if (!conn) {
                        close(cli);
                        continue;
                }

                struct epoll_event ev = {
                        .events = EPOLLIN,
                        .data.ptr = conn,
                };

                if (epoll_ctl(epfd, EPOLL_CTL_ADD, cli, &ev) < 0) {
                        log_error("epoll_ctl add client failed: %s\n", syserr);
                        connection_destroy(conn);
                }
        }
}

static void epoll_mark_writable(int epfd, struct connection *conn)
{
        if (!conn->writable) {
                conn->writable = 1;

                struct epoll_event ev = {
                        .events = EPOLLIN | EPOLLOUT | EPOLLET,
                        .data.ptr = conn,
                };

                if (epoll_ctl(epfd, EPOLL_CTL_MOD, conn->fd, &ev) < 0) {
                        log_error("mark connection %p writable failed: %s\n", conn, syserr);
                        connection_destroy(conn);
                }
        }
}

static void epoll_mark_unwritable(int epfd, struct connection *conn)
{
        if (conn->writable) {
                conn->writable = 0;

                struct epoll_event ev = {
                        .events = EPOLLIN | EPOLLET,
                        .data.ptr = conn,
                };

                if (epoll_ctl(epfd, EPOLL_CTL_MOD, conn->fd, &ev) < 0) {
                        log_error("mark connection %p unwritable failed: %s\n", conn, syserr);
                        connection_destroy(conn);
                }
        }
}

static ssize_t unpack(int epfd, struct connection *conn)
{
        ssize_t r;
        ipc_t ipc;
        ack_t ack;
        struct buffer *rb;

        rb = conn->rb;

        r = ipc_unpack(&ipc, rb->base + rb->rpos, buffer_length(rb));

        if (r > 0) {
                rb->rpos += r;

                ipc_ack(&ack, ipc.hdr.mid);

                log_debug("client connecetion %p recv message: %s\n", conn, ipc.data);

                if (connection_buffer_write(conn, &ack, sizeof(ack_t)) == 0)
                        epoll_mark_writable(epfd, conn);

                return r;
        }

        switch (r) {
                case -EINVAL:
                        log_error("invalid message protocol from %s\n", conn->addr.sin_addr);
                        return r;
                case -ENODATA:
                        return r;
                case -E2BIG:
                        log_error("message too large from %s\n", conn->addr.sin_addr);
                        return r;
                default:
                        log_error("unknown unpack error %ld from %s\n", r, conn->addr.sin_addr);
                        return r;
        }
}

static void on_event_read(int epfd, struct connection *conn)
{
        ssize_t r, err;

        while (true) {
                r = connection_socket_recv(conn);

                if (r > 0) {
                        err = unpack(epfd, conn);

                        if (err < 0 && err != -ENODATA)
                                goto err;

                        return;
                }

                if (r == -ENOSPC) {
                        err = unpack(epfd, conn);

                        if (err < 0) {
                                log_error("connection %p read buffer full but cannot consume buffer data\n",
                                          conn);
                                goto err;
                        }

                        continue;
                }

                goto err;
        }

err:
        connection_destroy(conn);
}

static void on_event_write(int epfd, struct connection *conn)
{
        if (connection_socket_send(conn) != 0)
                connection_destroy(conn);

        epoll_mark_unwritable(epfd, conn);
}

static void route_events(int epfd, int listen_fd,
                               struct epoll_event *events,
                               int nfds)
{
        struct epoll_event *cur_ev;
        struct connection *conn;

        for (int i = 0; i < nfds; i++) {
                cur_ev = &events[i];

                if (cur_ev->events & EPOLLRDHUP
                        || cur_ev->events & EPOLLHUP
                        || cur_ev->events & EPOLLERR) {
                        if (cur_ev->data.ptr) {
                                conn = (struct connection *) cur_ev->data.ptr;
                                epoll_ctl(epfd, EPOLL_CTL_DEL, conn->fd, NULL);
                                connection_destroy(conn);
                                continue;
                        }
                }

                if (cur_ev->data.fd == listen_fd) {
                        on_event_accept(epfd, listen_fd);
                        continue;
                }

                if (cur_ev->events & EPOLLIN)
                        on_event_read(epfd, (struct connection *) cur_ev->data.ptr);

                if (cur_ev->events & EPOLLOUT)
                        on_event_write(epfd, (struct connection *) cur_ev->data.ptr);
        }
}

__attr_noreturn
int main(int argc, char **argv)
{
        __attr_ignore2(argc, argv);

        if (argc > 1)
                client_start();

        int listen_fd;
        int epfd;

        struct epoll_event events[MAX_EVENTS];

        if (_init(&listen_fd, &epfd) != 0)
                exit(1);

        while (true) {
                int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);

                if (nfds < 0)
                        continue;

                route_events(epfd, listen_fd, events, nfds);
        }
}
