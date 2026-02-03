/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#include "connection.h"

#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>

#include "socket.h"
#include "utils/cntl.h"

#define RB_MAX   4096
#define WB_MAX   8192
#define IDLE_MAX 120

static void _conn_active(struct conn *c)
{
        c->last_active_ts = time(NULL);
}

struct conn *conn_create(int fd)
{
        struct conn *c;

        // if (isbadf(fd))
        //         return NULL;

        c = calloc(1, sizeof(struct conn));

        if (!c)
                return NULL;

        c->fd = fd;
        c->state = CONN_STATE_ALIVE;

        c->rb = buffer_alloc(RB_MAX);

        if (!c->rb)
                goto err;

        c->wb = buffer_alloc(WB_MAX);

        if (!c->wb)
                goto err;

        c->last_active_ts = time(NULL);
        c->idle_timeout_sec = IDLE_MAX;

        return c;

err:
        conn_destroy(c);
        return NULL;
}

void conn_destroy(struct conn *c)
{
        if (!c)
                return;

        if (c->state != CONN_STATE_CLOSED)
                conn_close(c);

        if (c->rb)
                buffer_free(c->rb);

        if (c->wb)
                buffer_free(c->wb);

        free(c);
}

void conn_close(struct conn *c)
{
        if (!c || c->state == CONN_STATE_CLOSED)
                return;

        // if (!isbadf(c->fd))
        //         close(c->fd);

        c->state = CONN_STATE_CLOSED;
}

void conn_reset(struct conn *c)
{
        c->state             = 0;

        c->rb->rpos         = 0;
        c->rb->wpos         = 0;

        c->wb->rpos         = 0;
        c->wb->wpos         = 0;

        c->last_active_ts    = 0;
        c->idle_timeout_sec  = 0;
}

int conn_write(struct conn *c, const void *data, size_t size)
{
        struct buffer *wb;
        size_t avail;

        wb = c->wb;
        avail = buffer_avail(wb);

        if (avail == 0 || size > avail)
                return -1;

        memcpy(wb->base + wb->wpos, data, size);

        wb->wpos += size;

        return 0;
}

conn_error_t conn_recv(struct conn *c)
{
        struct buffer *rb;
        size_t avail;
        ssize_t n;

        rb = c->rb;

        while (true) {
                avail = buffer_avail(rb);

                if (avail == 0)
                        return CONN_ERROR_NOBUFF;

                n = recv(c->fd, rb->base + rb->wpos, avail, 0);

                if (n > 0) {
                        rb->wpos += n;
                        _conn_active(c);
                        continue;
                }

                if (n == 0)
                        return CONN_ERROR_CLOSED;

                /* other syscall error */
                RETRY_IF_EINTR();

                if (is_eagain())
                        return CONN_OK;

                return CONN_ERROR_EOF;
        }
}

conn_error_t conn_send(struct conn *c)
{
        struct buffer *wb;
        size_t left;
        ssize_t n;

        wb = c->wb;

        while (true) {
                left = wb->wpos - wb->rpos;

                if (left == 0) {
                        buffer_compact(wb);

                        left = wb->wpos - wb->rpos;

                        if (left == 0)
                                return CONN_ERROR_NOBUFF;
                }

                n = send(c->fd, wb->base + wb->rpos, left, 0);

                if (n > 0) {
                        wb->rpos += n;
                        _conn_active(c);
                        continue;
                }

                if (n == 0)
                        return -1;

                /* other syscall error */
                RETRY_IF_EINTR();

                if (is_eagain())
                        return 0;

                return -1;
        }

        return 0;
}