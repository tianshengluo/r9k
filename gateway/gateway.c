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
#include "io/evlp.h"
#include "utils/log.h"
#include "ipc.h"
#include "config.h"

extern void client_start(void);

static evlp_t *_init(struct evlp_create_info *p_info)
{
        int listen_fd;
        evlp_t *evlp;

        listen_fd = tcp_create_listener(PORT);

        if (listen_fd < 0) {
                log_error("listen socket create failed: %s\n", syserr);
                exit(1);
                return NULL;
        }

        evlp = evlp_create(listen_fd, p_info);

        if (!evlp)
                exit(1);

        return evlp;
}

static ssize_t serialize_and_process(struct connection *conn)
{
        ssize_t r;
        uint64_t mid;
        char stack_buf[WB_MAX];

        r = ipc_proto_deserialize(conn->rb, stack_buf, sizeof(stack_buf));

        if (r > 0) {
                r = ipc_extract_and_valid(stack_buf, &mid);
                if (r != 0)
                        return -EINVAL;
                log_info("ipc recv body %s\n", stack_buf);
        }

        return 0;
}

static void on_event_read(evlp_t *evlp, struct connection *conn)
{
        __attr_ignore(evlp);

        ssize_t r, err;

        while (true) {
                r = connection_socket_recv(conn);

                if (r > 0) {
                        err = serialize_and_process(conn);

                        if (err < 0 && err != -ENODATA)
                                goto err;

                        return;
                }

                if (r == -ENOSPC) {
                        err = serialize_and_process(conn);

                        if (err < 0) {
                                log_error("connection %p from %s read buffer full but cannot consume buffer data\n",
                                          conn,
                                          conn->addr.sin_addr);
                                goto err;
                        }

                        continue;
                }

                goto err;
        }

err:
        evlp_close(evlp, conn);
}

static void on_event_write(evlp_t *evlp, struct connection *conn)
{
        if (connection_socket_send(conn) != 0)
                evlp_close(evlp, conn);

        evlp_disable_write(evlp, conn);
}

int main(int argc, char **argv)
{
        __attr_ignore2(argc, argv);

        if (argc > 1)
                client_start();

        evlp_t *evlp;

        struct evlp_create_info evlp_ci = {
                .on_read = on_event_read,
                .on_write = on_event_write,
        };

        evlp = _init(&evlp_ci);

        while (1)
             evlp_poll_events(evlp);
}
