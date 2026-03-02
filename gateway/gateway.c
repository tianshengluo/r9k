/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2025 Luo Tiansheng
 */
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <r9k/compiler_attrs.h>
#include "io/connection.h"
#include "io/socket.h"
#include "io/evlp.h"
#include "utils/log.h"
#include "fip.h"
#include "config.h"

extern void client_start(void);

static evlp_t *_init(struct evlp_create_info *p_info)
{
        int listen_fd;
        evlp_t *evlp;

        listen_fd = tcp_create_listener(GW_SERVER_PORT);

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

static ssize_t _ack_serialize_and_process(struct buffer *rb)
{
        ack_t ack;
        return ack_packet_deserialize(rb, &ack);
}

static int _fip_recv_auth(struct connection *conn, fip_t *fip, char *tlv)
{
        if (fip->type != FIP_AUTHORIZE)
                return -EINVAL;

        log_info("jwt: %s\n", tlv);
        conn->is_auth = 1;

        return 0;
}

static int _fip_recv_message(struct connection *conn, fip_t *fip, char *tlv)
{
        __attr_ignore2(conn, fip);

        int r;
        uint64_t mid;

        r = fip_extract_and_valid(tlv, &mid);

        if (r != 0)
                return -EINVAL;

        log_info("fip recv tlv: %s\n", tlv);

        return r;
}

static ssize_t serialize_and_process(struct connection *conn)
{
        ssize_t r;
        fip_t fip;
        char tlv[MAX_WB];
        struct buffer *rb = conn->rb;

        r = _ack_serialize_and_process(rb);

        if (r == 0)
                return 0;

        /* parse fimp packet */
        r = fip_packet_deserialize(rb, &fip, tlv, sizeof(tlv));

        if (r < 0)
                return r;

        if (!conn->is_auth && fip.type != FIP_AUTHORIZE)
                return -EPROTO;

        switch (fip.type) {
                case FIP_AUTHORIZE:
                        return _fip_recv_auth(conn, &fip, tlv);
                case FIP_MESSAGE:
                        return _fip_recv_message(conn, &fip, tlv);
                default:
                        log_error("fip unknown proto packet type: %u\n", fip.type);
                        return -EPROTOTYPE;
        }
}

static void on_event_accept(evlp_t *evlp, struct connection *conn)
{
        __attr_ignore2(evlp, conn);
}

static void on_event_close(evlp_t *evlp, struct connection *conn)
{
        __attr_ignore2(evlp, conn);
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
                .on_accept = on_event_accept,
                .on_close = on_event_close,
                .on_read = on_event_read,
                .on_write = on_event_write,
        };

        evlp = _init(&evlp_ci);

        while (1)
             evlp_poll_events(evlp);
}
