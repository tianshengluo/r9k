/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#ifndef CONNECTION_H_
#define CONNECTION_H_

#include <stdint.h>
#include <stddef.h>

typedef uint8_t *byte_ptr_t;
typedef struct buffer buffer_t;

typedef enum {
        CONN_STATE_INIT = 0,
        CONN_STATE_ESTABLISHED,
        CONN_STATE_CLOSED,
} connstate_t;

struct connection {
        int             fd;
        buffer_t*       _rb;
        buffer_t*       _wb;
        connstate_t     state;
        uint64_t        last_active_ts;
        uint32_t        idle_timeout_sec;
};

struct connection *connection_create(int fd);
void connection_destroy(struct connection *conn);
void connection_close(struct connection *conn);
void connection_reset(struct connection *conn);

void connection_rbuf_read(struct connection *conn);
byte_ptr_t connection_rbuf_peek(struct connection *conn, size_t off);
void connection_rbuf_consume(struct connection *conn, size_t off);

#endif /* CONNECTION_H_ */