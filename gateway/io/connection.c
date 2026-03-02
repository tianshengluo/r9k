/*
-* SPDX-License-Identifier: MIT
 * Copyright (C) 2025 Luo Tiansheng
 */
#define CONNECTION_EXTERN_FUNC
#include "connection.h"

#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>

#include "socket.h"
#include "utils/log.h"
#include "config.h"

static void _connection_mark_active(struct connection *conn)
{
        conn->last_active_ts = time(NULL);
}

__attr_unused
static void _connection_reset(struct connection *conn)
{
        conn->rb->rpos         = 0;
        conn->rb->wpos         = 0;

        conn->wb->rpos         = 0;
        conn->wb->wpos         = 0;

        conn->last_active_ts   = 0;
        conn->idle_timeout_sec = 0;
}

void connection_close(struct connection *conn)
{
        if (conn->state == CONN_STATE_CLOSING)
                return;

        conn->state = CONN_STATE_CLOSING;

        close(conn->fd);
        conn->fd = -1;
}

void connection_destroy(struct connection *conn)
{
        if (!conn)
                return;

        connection_close(conn);

        if (conn->rb)
                buffer_free(conn->rb);

        if (conn->wb)
                buffer_free(conn->wb);

        free(conn);
}

struct connection *connection_create(int fd, struct host_sockaddr_in *addr)
{
        struct connection *conn;

        conn = calloc(1, sizeof(struct connection));

        if (!conn)
                return NULL;

        conn->fd = fd;
        conn->state = CONN_STATE_ACTIVE;

        conn->rb = buffer_alloc(MAX_RB);

        if (!conn->rb)
                goto err;

        conn->wb = buffer_alloc(MAX_WB);

        if (!conn->wb)
                goto err;

        conn->last_active_ts = time(NULL);
        conn->idle_timeout_sec = MAX_IDL;

        memcpy(&conn->addr, addr, sizeof(struct host_sockaddr_in));

        return conn;

err:
        connection_destroy(conn);
        return NULL;
}

void connection_set_userdata(struct connection *conn, void *udata)
{
        conn->udata = udata;
}

void *connection_get_userdata(struct connection *conn)
{
        return conn->udata;
}

ssize_t connection_buffer_write(struct connection *conn, const void *data, size_t size)
{
        struct buffer *wb;
        size_t avail;

        wb = conn->wb;
        avail = buffer_writeable(wb);

        if (avail == 0 || size > avail)
                return -ENOBUFS;

        memcpy(buffer_peek_wcur(wb), data, size);

        buffer_skip_wpos(wb, size);

        return size;
}

ssize_t connection_socket_recv(struct connection *conn)
{
        struct buffer *rb;
        size_t avail;
        ssize_t n;
        ssize_t total = 0;

        rb = conn->rb;

        while (true) {
                avail = buffer_writeable(rb);

                if (avail == 0)
                        return -ENOSPC;


                n = recv(conn->fd, buffer_peek_wcur(rb), avail, 0);

                if (n > 0) {
                        total += n;
                        buffer_skip_wpos(rb, n);
                        _connection_mark_active(conn);
                        continue;
                }

                if (n == 0)
                        return 0;

                /* other syscall error */
                RETRY_IF_EINTR();

                if (is_eagain())
                        return total;

                if (errno == ECONNRESET)
                        return -errno;

                return -EIO;
        }
}

ssize_t connection_socket_send(struct connection *conn)
{
        struct buffer *wb;
        size_t left;
        ssize_t n;

        wb = conn->wb;

        while (true) {
                left = buffer_readable(wb);

                if (left == 0)
                        return 0;

                n = send(conn->fd, buffer_peek_rcur(wb), left, 0);

                if (n > 0) {
                        buffer_skip_rpos(wb, n);
                        _connection_mark_active(conn);
                        continue;
                }

                if (n == 0)
                        return -errno;

                /* other syscall error */
                RETRY_IF_EINTR();

                if (is_eagain())
                        return 0;

                return -errno;
        }
}