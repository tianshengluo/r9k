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
#include "server.h"

static void process_eim(struct eim *proto)
{

}

static void read_event(struct connection *conn)
{
        if (connection_read(conn) < 0)
                return;

        size_t size;
        eim_error_t r;
        struct eim proto;
        struct stagbuf *rb = &conn->rb;

        while (rb->len >= EIM_SIZE) {
                r = eim(rb->buf, rb->len, &size, &proto);

                switch (r) {
                        case EIM_ERROR_INCOMPLETE:
                                break;
                        case EIM_OK:
                                process_eim(&proto);
                                rb->len -= size;
                                continue;
                        default:
                                connection_destroy(conn);
                                return;
                }

                break;
        }
}

static void write_event(struct connection *conn)
{

}

__attr_noreturn
int main(int argc, char **argv)
{
        __attr_ignore2(argc, argv);

        struct server *srv;

        srv = server_start(26214);

        if (!srv) {
                logger_error("server start failed, cause: %s\n", syserr);
                exit(1);
        }

        server_set_read_event(srv, read_event);
        server_set_write_event(srv, write_event);

        while (1)
                server_poll_events(srv);
}
