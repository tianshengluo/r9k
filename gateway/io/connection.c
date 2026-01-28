/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#include "connection.h"

#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>

#include "socket.h"
#include "utils/cntl.h"

#define RB_MAX   4096
#define WB_MAX   8192
#define IDLE_MAX 120

struct buffer {
        byte_ptr_t      base;
        size_t          cap;
        size_t          rpos;
        size_t          wpos;
};

static buffer_t *buffer_alloc(size_t size)
{
        buffer_t *buffer;

        buffer = calloc(1, sizeof(struct buffer));

        if (!buffer)
                return NULL;

        buffer->base = malloc(size);

        if (!buffer->base) {
                free(buffer);
                return NULL;
        }

        return buffer;
}

static void buffer_free(buffer_t *buffer)
{
        if (!buffer)
                return;

        if (buffer->base)
                free(buffer->base);

        free(buffer);
}

struct connection *connection_create(int fd)
{
        struct connection *conn;

        if (isbadf(fd))
                return NULL;

        conn = calloc(1, sizeof(struct connection));

        if (!conn)
                return NULL;

        conn->fd = fd;
        conn->state = CONN_STATE_ESTABLISHED;

        conn->_rb = buffer_alloc(RB_MAX);

        if (!conn->_rb)
                goto err;

        conn->_wb = buffer_alloc(WB_MAX);

        if (!conn->_wb)
                goto err;

        conn->last_active_ts = time(NULL);
        conn->idle_timeout_sec = IDLE_MAX;

        return conn;

err:
        connection_destroy(conn);
        return NULL;
}

void connection_destroy(struct connection *conn)
{
        if (!conn)
                return;

        if (conn->state != CONN_STATE_CLOSED)
                connection_close(conn);

        if (conn->_rb)
                buffer_free(conn->_rb);

        if (conn->_wb)
                buffer_free(conn->_wb);

        free(conn);
}

void connection_close(struct connection *conn)
{
        if (!conn)
                return;

        if (!isbadf(conn->fd))
                close(conn->fd);

        /* reset */
        connection_reset(conn);

        conn->state = CONN_STATE_CLOSED;
}

void connection_reset(struct connection *conn)
{
        conn->state             = 0;

        conn->_rb->rpos         = 0;
        conn->_rb->wpos         = 0;

        conn->_wb->rpos         = 0;
        conn->_wb->wpos         = 0;

        conn->last_active_ts    = 0;
        conn->idle_timeout_sec  = 0;
}

void connection_rbuf_read(struct connection *conn)
{
        buffer_t *rb = conn->_rb;

        while (1) {
                size_t avail = rb->cap - rb->wpos;

                if (avail == 0)
                        goto close;

                ssize_t n = recv(conn->fd, rb->base + rb->wpos, avail, 0);

                if (n == 0)
                        goto close;

                if (n < 0) {
                        RETRY_IF_EINTR();

                        if (is_eagain())
                                break;

                        goto close;
                }

                rb->wpos += n;
        }

close:
        connection_close(conn);
}

byte_ptr_t connection_rbuf_peek(struct connection *conn, size_t off)
{
        buffer_t *rb = conn->_rb;

        if (rb->wpos - rb->rpos < off)
                return NULL;

        return rb->base + rb->rpos + off;
}

void connection_rbuf_consume(struct connection *conn, size_t off)
{
        buffer_t *rb = conn->_rb;

        size_t avail = rb->wpos - rb->rpos;

        if (off > avail)
                off = avail;

        rb->rpos += off;

        if (rb->rpos == rb->wpos)
                rb->rpos = rb->wpos = 0;
}