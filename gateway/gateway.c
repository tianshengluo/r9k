/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <r9k/compiler_attrs.h>

#include "reactor.h"
#include "log.h"
#include "eim.h"
#include "config.h"

extern void client_start();

static ssize_t eim_parse_proto(stagbuf_t *rb)
{
        if (rb->len < EIM_SIZE)
                return EIM_ERROR_INCOMPLETE;

        ssize_t size;
        eim_t *proto;

        size = eim(rb->buf, rb->len, &proto);

        if (size <= 0)
                return size;

        char *data = (char *) rb->buf + EIM_SIZE;
        logger_info("EIM receiver from client message: %.*s\n", (int) proto->len, data);

        return size;
}

static void on_event_read(connection_t *conn)
{
        stagbuf_t *rb = &conn->rb;

        while (1) {
                ssize_t n = recv(conn->fd,
                                 rb->buf + rb->len,
                                 rb->cap - rb->len,
                                 0);

                if (n < 0) {
                        RETRY_ONCE_ENTR();

                        if (is_eagain())
                                return;
                }

                if (n == 0) {
                        connection_destroy(conn);
                        return;
                }

                rb->len += n;

                while (is_eim(rb->len)) {
                        /* parse proto */
                        n = eim_parse_proto(rb);

                        if (n < 0) {
                                if (n == EIM_ERROR_INCOMPLETE)
                                        break;
                                connection_destroy(conn);
                                return;
                        }

                        rb->len -= n;
                }
        }
}

static void on_event_write(connection_t *conn)
{
        __attr_ignore(conn);
}

int main(int argc, char **argv)
{
        __attr_ignore2(argc, argv);

        if (argc > 1) {
                client_start();
                exit(0);
        }

        int listen_fd;
        reactor_t *rc;

        listen_fd = tcp_create_listener(PORT);

        if (listen_fd < 0) {
                logger_error("listen fd open failed, cause: %s\n", syserr);
                exit(1);
        }

        rc = rc_create(listen_fd, 1024);

        if (!rc) {
                logger_error("reactor create failed, cause: %s\n", syserr);
                close(listen_fd);
                exit(1);
        }

        logger_info("server start successful, fd=%d, listening port %d\n", listen_fd, PORT);

        rc_set_read_callback(rc, on_event_read);
        rc_set_write_callback(rc, on_event_write);

        while (1)
                rc_poll_events(rc);
}
