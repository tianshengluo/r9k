/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#ifndef _conn_H
#define _conn_H

#include <stdint.h>
#include <time.h>

#include "buffer.h"

typedef enum {
        CONN_STATE_NEW,
        CONN_STATE_ALIVE,
        CONN_STATE_CLOSED,
} conn_state_t;

typedef enum {
        CONN_OK = 0,
        CONN_ERROR_CLOSED = -1,
        CONN_ERROR_NOBUFF = -2,
        CONN_ERROR_EOF    = -3,
} conn_error_t;

struct conn {
        int             fd;
        struct buffer  *rb;
        struct buffer  *wb;
        conn_state_t    state;
        time_t          last_active_ts;
        uint32_t        idle_timeout_sec;
};

struct conn *conn_create(int fd);
void conn_destroy(struct conn *c);

void conn_reset(struct conn *c);
void conn_close(struct conn *c);

/* Return 0 mean success, it's either a success or a failure */
int conn_write(struct conn *c, const void *data, size_t size);

conn_error_t conn_recv(struct conn *c);
conn_error_t conn_send(struct conn *c);

#endif /* _conn_H */