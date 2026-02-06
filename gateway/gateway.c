/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <r9k/compiler_attrs.h>
#include <r9k/yyjson.h>

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

        log_info("server start success listening on port: %d\n", PORT);

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

static uint64_t ipc_body_valid(const char *data)
{
        yyjson_doc *doc;
        yyjson_val *root;
        yyjson_val *obj;

        const char *content;
        uint64_t mid;

        doc = yyjson_read(data, strlen(data), YYJSON_READ_NOFLAG);

        if (!doc) {
                log_error("parse ipc data json error");
                goto err;
        }

        root = yyjson_doc_get_root(doc);

        if (yyjson_get_type(root) != YYJSON_TYPE_OBJ) {
                log_error("json root node is not object type");
                goto err;
        }

        obj = yyjson_obj_get(root, "content");

        if (!obj || !yyjson_get_type(obj) == YYJSON_TYPE_STR) {
                log_error("json content is null or not string");
                goto err;
        }

        content = yyjson_get_str(obj);

        if (strlen(content) == 0) {
                log_error("message content cannot empty");
                goto err;
        }

        obj = yyjson_obj_get(root, "mid");

        if (!obj || !yyjson_get_type(obj) == YYJSON_TYPE_NUM) {
                log_error("json mid is null or not num");
                goto err;
        }

        mid = yyjson_get_uint(obj);
        yyjson_doc_free(doc);

        return mid;

err:
        yyjson_doc_free(doc);
        return -EINVAL;
}

static ssize_t try_unpack_ipc(int epfd, struct connection *conn)
{
        ipc_t ipc;
        ssize_t r;
        struct buffer *rb = conn->rb;
        uint8_t *start_buf;
        uint64_t mid;

        while (true) {
                start_buf = rb->base + rb->rpos;

                r = ipc_header_unpack(&ipc, start_buf, buffer_readable(rb));

                if (r > 0) {
                        char *data = (char *) (start_buf + r);
                        rb->rpos += r + ipc.body_len;
                        mid = ipc_body_valid(data);
                        log_info("recv client from %s ipc data[%llu]: %s\n",
                                 conn->addr.sin_addr,
                                 mid,
                                 data);
                        continue;
                }

                switch (r) {
                        case -EPROTO:
                                log_error("invalid protocol data, parse ipc_t failed\n");
                                goto err;
                        case -ENODATA:
                                goto err;
                        default:
                                log_error("unknown ipc_unpack_buffer() return errno: %ld\n", r);
                                goto err;
                }
        }

        return 0;

err:
        return -1;
}

static void on_event_read(int epfd, struct connection *conn)
{
        ssize_t r, err;

        while (true) {
                r = connection_socket_recv(conn);

                if (r > 0) {
                        err = try_unpack_ipc(epfd, conn);

                        if (err < 0 && err != -ENODATA)
                                goto err;

                        return;
                }

                if (r == -ENOSPC) {
                        err = try_unpack_ipc(epfd, conn);

                        if (err < 0) {
                                log_error("connection %p read buffer full but cannot consume buffer data\n", conn);
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
