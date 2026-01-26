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
        logger_info("recv from client message: %.*s\n", (int) proto->len, data);

        return size;
}

static int check_packet_ready(connection_t *conn, stagbuf_t *rb)
{
        while (is_eim_ready(rb->len)) {
                /* parse proto */
                ssize_t n = eim_parse_proto(rb);

                if (n < 0) {
                        if (n == EIM_ERROR_INCOMPLETE)
                                break;
                        connection_destroy(conn);
                        return -1;
                }

                /* write ack */
                eim_t e_ack;
                ack(&e_ack);

                connection_write(conn, &conn->wb0, &e_ack, ACK_SIZE);

                rb->len -= n;
        }

        return 0;
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
                        RETRY_IF_EINTR();

                        if (is_eagain())
                                return;

                        goto close;
                }

                if (n == 0)
                        goto close;

                rb->len += n;

                if (check_packet_ready(conn, rb) < 0)
                        return;
        }

close:
        connection_destroy(conn);
}

__attr_noreturn
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

        while (1)
                rc_poll_events(rc);
}
