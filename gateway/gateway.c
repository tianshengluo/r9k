/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <r9k/error.h>
#include <r9k/compiler_attrs.h>

#include "socket.h"
#include "connection.h"

#define SOCK_PORT 26214
#define MAX_EVENTS 114514
#define MAX_ACCEPT 128

static void handle_accept(int epfd, int listen_fd)
{
        int cli;
        struct connection *conn;

        for (int i = 0; i < MAX_ACCEPT; i++) {
                cli = socket_accept(listen_fd, NULL);

                if (cli < 0) {
                        if (is_eagain())
                                break;
                        continue;
                }

                set_nonblock(cli);

                conn = connection_new(cli, 8192);

                if (!conn) {
                        close(cli);
                        continue;
                }

                struct epoll_event ev = {
                        .events = EPOLLIN | EPOLLET,
                        .data.ptr = conn,
                };

                if (epoll_ctl(epfd, EPOLL_CTL_ADD, cli, &ev) < 0)
                        connection_close(conn);
        }
}

static void handle_events(int epfd, int listen_fd, struct epoll_event *events, int count)
{
        struct connection *conn;

        for (int i = 0; i < count; i++) {
                conn = (struct connection *) events[i].data.ptr;

                if (conn->fd == listen_fd) {
                        handle_accept(epfd, conn->fd);
                        continue;
                }
        }
}

__attr_noreturn
void main(int argc, char **argv)
{
        __attr_ignore2(argc, argv);

        int listen_fd;
        int epfd;

        listen_fd = socket_start(SOCK_PORT);

        if (listen_fd < 0)
                FATAL();

        /* set nonblock */
        set_nonblock(listen_fd);

        epfd = epoll_create1(EPOLL_CLOEXEC);

        if (epfd < 0)
                FATAL();

        struct connection *conn;

        conn = connection_wrap(listen_fd);

        if (!conn)
                FATAL();

        struct epoll_event event = {
                .events = EPOLLIN,
                .data.ptr = conn,
        };

        if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &event) < 0)
                FATAL();

        struct epoll_event events[MAX_EVENTS];

        while (1) {
                int n = epoll_wait(epfd, events, MAX_EVENTS, -1);

                if (n >= 0)
                        handle_events(epfd, listen_fd, events, n);
        }
}
