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
#include "evlp.h"

extern void client_start(void);

static evlp_t *_init(on_read_fn_t on_read, on_write_fn_t on_write)
{
        int listen_fd;
        evlp_t *evlp;

        listen_fd = tcp_create_listener(PORT);

        if (listen_fd < 0) {
                log_error("listen socket create failed: %s\n", syserr);
                exit(1);
                return NULL;
        }

        evlp = evlp_create(listen_fd, on_read, on_write);

        if (!evlp)
                exit(1);

        return evlp;
}

static uint64_t ipc_body_valid(const char *data)
{
        yyjson_doc *doc;
        yyjson_val *root;
        yyjson_val *obj_mid;
        yyjson_val *obj_content;

        const char *content;
        uint64_t mid;

        doc = yyjson_read(data, strlen(data), YYJSON_READ_NOFLAG);

        if (!doc) {
                log_error("JSON: parse ipc data error\n");
                goto err;
        }

        root = yyjson_doc_get_root(doc);

        if (yyjson_get_type(root) != YYJSON_TYPE_OBJ) {
                log_error("JSON: root node is not object type");
                goto err;
        }

        obj_content = yyjson_obj_get(root, "msg_content");

        if (!obj_content || !yyjson_get_type(obj_content) == YYJSON_TYPE_STR)
                goto err;

        content = yyjson_get_str(obj_content);

        if (strlen(content) == 0) {
                log_error("message content cannot empty\n");
                goto err;
        }

        obj_mid = yyjson_obj_get(root, "msg_id");

        if (!obj_mid || !yyjson_get_type(obj_mid) == YYJSON_TYPE_NUM) {
                log_error("JSON: mid is null or not num\n");
                goto err;
        }

        mid = yyjson_get_uint(obj_mid);
        yyjson_doc_free(doc);

        return mid;

err:
        yyjson_doc_free(doc);
        return -EINVAL;
}

static void sendack(evlp_t *evlp, struct connection * conn, uint64_t mid)
{
        ack_t ak;
        ssize_t r;

        ack(&ak, mid);

        r = connection_buffer_write(conn, &ak, sizeof(ack_t));

        if (r < 0)
                return;

        evlp_mark_writable(evlp, conn);
}

static ssize_t try_unpack_ipc(evlp_t *evlp, struct connection *conn)
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
                        sendack(evlp, conn, mid);
                        continue;
                }

                switch (r) {
                        case -EPROTO:
                                log_error("invalid protocol data, parse ipc_t failed\n");
                                goto err;
                        case -ENODATA:
                                return -ENODATA;
                        default:
                                log_error("unknown ipc_unpack_buffer() return errno: %ld\n", r);
                                goto err;
                }
        }

        return 0;

err:
        return -1;
}

static void on_event_read(evlp_t *evlp, struct connection *conn)
{
        ssize_t r, err;

        while (true) {
                r = connection_socket_recv(conn);

                if (r > 0) {
                        err = try_unpack_ipc(evlp, conn);

                        if (err < 0 && err != -ENODATA)
                                goto err;

                        return;
                }

                if (r == -ENOSPC) {
                        err = try_unpack_ipc(evlp, conn);

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

static void on_event_write(evlp_t *evlp, struct connection *conn)
{
        if (connection_socket_send(conn) != 0)
                connection_destroy(conn);

        evlp_mark_unwritable(evlp, conn);
}

int main(int argc, char **argv)
{
        __attr_ignore2(argc, argv);

        if (argc > 1)
                client_start();

        evlp_t *evlp;

        evlp = _init(on_event_read, on_event_write);

        while (1)
             evlp_poll_events(evlp);
}
