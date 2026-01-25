/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <r9k/compiler_attrs.h>

#include "log.h"
#include "socket.h"
#include "conn.h"
#include "eim.h"

#define PORT       1280
#define MAX_EVENTS 128
#define MAX_ACCEPT 128
#define MAX_RB     4096
#define MAX_WB     8192

static void accept_connection(int epfd, int listen_fd)
{
        int cli;
        struct connection *conn;

        for (int i = 0; i < MAX_ACCEPT; i++) {
                cli = tcp_accept(listen_fd, NULL);

                if (cli < 0) {
                        RETRY_ONCE_ENTR();

                        if (is_eagain())
                                break;
                }

                conn = connection_create(cli, MAX_RB, MAX_WB);

                if (!conn) {
                        close(cli);
                        LOG_ERROR("connection create failed, fd: %d, cause: %s\n", cli, syserr);
                        continue;
                }

                struct epoll_event event = {
                        .events = EPOLLIN | EPOLLET,
                        .data.ptr = conn,
                };

                if (epoll_ctl(epfd, EPOLL_CTL_ADD, cli, &event) < 0) {
                        LOG_WARN("epoll add client connection failed, cause: %s\n", syserr);
                        close(cli);
                        continue;
                }
        }
}

static void process_read(int epfd, struct connection *conn)
{
        struct stagbuf *rb = &conn->rb;
        struct stagbuf *wb = &conn->wb;

        while (1) {
                ssize_t n = recv(conn->fd,
                                 rb->buf + rb->len,
                                 rb->cap - rb->len,
                                 0);

                if (n == 0) {
                        connection_destroy(conn);
                        break;
                }

                if (n < 0) {
                        RETRY_ONCE_ENTR();

                        if (is_eagain())
                                break;
                }

                rb->len += n;

                while (rb->len >= EIM_SIZE) {
                        /* parse proto */
                }
        }
}

static void process_events(int epfd, int listen_fd, int count, struct epoll_event *events)
{
        for (int i = 0; i < count; i++) {
                /* new connection */
                if (events[i].data.fd == listen_fd) {
                        accept_connection(epfd, listen_fd);
                        continue;
                }

                /* read event */
                process_read(epfd, (struct connection *) events[i].data.ptr);
        }
}

__attr_noreturn
int main(int argc, char **argv)
{
        __attr_ignore2(argc, argv);

        int listen_fd;

        listen_fd = tcp_create_listener(PORT);

        if (listen_fd < 0) {
                LOG_ERROR("tcp_create_listener() failed, cause: %s\n", syserr);
                exit(1);
        }

        int epfd;

        epfd = epoll_create1(EPOLL_CLOEXEC);

        if (epfd < 0) {
                LOG_ERROR("epoll create failed, cause: %s\n", syserr);
                close(listen_fd);
                exit(1);
        }

        struct epoll_event event = {
                .events = EPOLLIN,
                .data.fd = listen_fd
        };

        if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &event) < 0) {
                LOG_ERROR("epoll add listen_fd failed, cause: %s\n", syserr);
                close(listen_fd);
                close(epfd);
                exit(1);
        }

        int n;
        struct epoll_event events[MAX_EVENTS];

        while (1) {
                n = epoll_wait(epfd, events, MAX_EVENTS, -1);

                if (n > 0)
                        process_events(epfd, listen_fd, n, events);
        }

}
