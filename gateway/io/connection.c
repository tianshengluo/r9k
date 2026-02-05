/*
-* SPDX-License-Identifier: MIT
 * Copyright (conn) 2025
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
#include "utils/log.h"

#define RB_MAX   128
#define WB_MAX   8192
#define IDLE_MAX 120

static void _connection_active(struct connection *conn)
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

struct connection *connection_create(int fd, struct host_sockaddr_in *addr)
{
        struct connection *conn;

        if (isbadf(fd))
                return NULL;

        conn = calloc(1, sizeof(struct connection));

        if (!conn)
                return NULL;

        conn->fd = fd;

        conn->rb = buffer_alloc(RB_MAX);

        if (!conn->rb)
                goto err;

        conn->wb = buffer_alloc(WB_MAX);

        if (!conn->wb)
                goto err;

        conn->last_active_ts = time(NULL);
        conn->idle_timeout_sec = IDLE_MAX;

        memcpy(&conn->addr, addr, sizeof(struct host_sockaddr_in));

        log_debug("accept connection %p, fd=%d, rb=%p, wb=%p\n", conn, fd, conn->rb, conn->wb);

        return conn;

err:
        connection_destroy(conn);
        return NULL;
}

void connection_destroy(struct connection *conn)
{
        if (!conn)
                return;

        log_debug("destroying connection %p\n", conn);

        if (!isbadf(conn->fd))
                close(conn->fd);

        if (conn->rb)
                buffer_free(conn->rb);

        if (conn->wb)
                buffer_free(conn->wb);

        free(conn);
}

void connection_close(struct connection *conn)
{
        if (!isbadf(conn->fd))
                close(conn->fd);
}

int connection_buffer_write(struct connection *conn, const void *data, size_t size)
{
        struct buffer *wb;
        size_t avail;

        wb = conn->wb;
        avail = buffer_writeable(wb);

        if (avail == 0 || size > avail)
                return -EINVAL;

        memcpy(wb->base + wb->wpos, data, size);

        wb->wpos += size;

        return 0;
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

                n = recv(conn->fd, rb->base + rb->wpos, avail, 0);

                if (n > 0) {
                        total += n;
                        rb->wpos += n;
                        _connection_active(conn);
                        continue;
                }

                if (n == 0) {
                        log_debug("connection %p recv close\n", conn);
                        return 0;
                }

                /* other syscall error */
                RETRY_IF_EINTR();

                if (is_eagain())
                        return total;

                if (errno == ECONNRESET) {
                        log_debug("connection %p reset by peer\n", conn);
                        return -errno;
                }

                log_debug("connection %p recv failed, cause: %s\n", conn, syserr);

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
                left = wb->wpos - wb->rpos;

                if (left == 0)
                        return 0;

                n = send(conn->fd, wb->base + wb->rpos, left, 0);

                if (n > 0) {
                        wb->rpos += n;
                        _connection_active(conn);
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