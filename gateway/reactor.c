/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 *
 * Enterprise Instant Messaging.
 */
#include "reactor.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "log.h"

#define MAX_RB  (1024 * 4)
#define MAX_WB0 (1024 * 4)
#define MAX_WB1 (1024 * 8)

static uint32_t to_epoll_events(uint32_t events)
{
        uint32_t ep_events = 0;

        if (events & RC_READ)
                ep_events |= EPOLLIN;

        if (events & RC_WRITE)
                ep_events |= EPOLLOUT;

        if (events & RC_ET)
                ep_events |= EPOLLET;

        return ep_events | EPOLLHUP | EPOLLRDHUP | EPOLLERR;
}

static int rc_event_add(reactor_t *rc, int fd, uint32_t events, connection_t *conn)
{
        struct epoll_event ev = {
                .events = to_epoll_events(events),
                .data.fd = fd,
        };

        if (conn) {
                ev.data.ptr = conn;
                conn->flags = events;
        }

        return epoll_ctl(rc->epfd, EPOLL_CTL_ADD, fd, &ev);
}

static int rc_event_mod(reactor_t *rc, int fd, uint32_t events, connection_t *conn)
{
        struct epoll_event ev = {
                .events = to_epoll_events(events),
                .data.fd = fd,
        };

        if (conn) {
                ev.data.ptr = conn;
                conn->flags = events;
        }

        return epoll_ctl(rc->epfd, EPOLL_CTL_MOD, fd, &ev);
}

static int rc_event_del(reactor_t *rc, int fd)
{
        return epoll_ctl(rc->epfd, EPOLL_CTL_DEL, fd, NULL);
}

static void rc_accept_connection(reactor_t *rc)
{
        int cli;
        connection_t *conn;

        while (1) {
                cli = tcp_accept(rc->listen_fd, NULL);

                if (cli < 0) {
                        int err = errno;

                        RETRY_IF_EINTR();

                        if (is_eagain())
                                break;

                        logger_error("accept failed on listen fd: %d, errno=%d (%s)\n",
                                     rc->listen_fd, err, strerror(err));

                        if (err == EBADF || err == EINVAL || err == ENOTSOCK) {
                                logger_fatal("listen socket invalid, stopping acceptor");
                                _exit(1);
                        }
                }

                conn = connection_create(rc, cli, MAX_RB, MAX_WB0, MAX_WB1);

                if (!conn) {
                        close(cli);
                        continue;
                }

                if (rc_event_add(rc, cli, RC_READ | RC_ET, conn) < 0)
                        connection_destroy(conn);
        }
}

static int rc_buffer_write(connection_t *conn, stagbuf_t *wb)
{
        if (wb->len == 0)
                return 0;

        ssize_t nwrite = 0;

        while (nwrite < wb->len) {
                ssize_t n = send(conn->fd,
                                 wb->buf + nwrite,
                                 wb->len - nwrite,
                                 0);

                if (n == 0)
                        return -1;

                if (n < 0) {
                        RETRY_IF_EINTR();

                        if (is_eagain())
                                return 0;

                        return -1;
                }

                nwrite += n;
                wb->len -= n;
        }

        return 0;
}

static void rc_on_event_write(connection_t *conn)
{
        if (rc_buffer_write(conn, &conn->wb0) != 0)
                goto close;

        if (rc_buffer_write(conn, &conn->wb1) != 0)
                goto close;

        if (conn->wb0.len == 0 && conn->wb1.len == 0) {
                if (rc_event_mod(conn->rc, conn->fd, RC_READ | RC_ET, conn) < 0)
                        goto close;
        }

        return;

close:
        connection_destroy(conn);
}

static void rc_events_dispatch(reactor_t *rc)
{
        struct epoll_event *curev;

        for (int i = 0; i < rc->event_count; i++) {
                curev = &rc->event_array[i];

                if (curev->data.fd == rc->listen_fd) {
                        rc_accept_connection(rc);
                        continue;
                }

                if (curev->events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
                        connection_destroy((connection_t *) curev->data.ptr);
                        continue;
                }

                if (curev->events & EPOLLIN)
                        rc->on_event_read((connection_t *) curev->data.ptr);

                if (curev->events & EPOLLOUT)
                        rc_on_event_write((connection_t *) curev->data.ptr);
        }
}

reactor_t *rc_create(int listen_fd, size_t maxevents)
{
        reactor_t *rc;

        rc = calloc(1, sizeof(reactor_t));

        if (!rc)
                return NULL;

        rc->maxevents = maxevents;
        rc->listen_fd = listen_fd;

        rc->epfd = epoll_create1(EPOLL_CLOEXEC);

        if (rc->epfd < 0)
                goto err;

        rc->event_array = malloc(maxevents * sizeof(struct epoll_event));

        if (!rc->event_array)
                goto err;

        if (rc_event_add(rc, listen_fd, RC_READ | RC_ET, NULL) < 0)
                goto err;

        return rc;

err:
        rc_destroy(rc);
        return NULL;
}

void rc_destroy(reactor_t *rc)
{
        if (!rc)
                return;

        if (rc->event_array)
                free(rc->event_array);

        if (!isbadf(rc->epfd))
                close(rc->epfd);

        free(rc);
}

void rc_set_read_callback(reactor_t *rc, fn_on_event on_event_read)
{
        if (on_event_read)
                rc->on_event_read = on_event_read;
}

void rc_poll_events(reactor_t *rc)
{
        int nfds;

        nfds = epoll_wait(rc->epfd, rc->event_array, (int) rc->maxevents, -1);

        if (nfds > 0) {
                rc->event_count = nfds;
                rc_events_dispatch(rc);
        }
}

connection_t *connection_create(reactor_t *rc, int fd, size_t maxrb, size_t maxwb0, size_t maxwb1)
{
        connection_t *conn;

        conn = calloc(1, sizeof(connection_t));

        if (!conn)
                return NULL;

        conn->rc = rc;

        /* read buffer */
        conn->rb.buf = malloc(maxrb);
        conn->rb.cap = maxrb;

        if (!conn->rb.buf)
                goto err;

        /* write buffer 0 */
        conn->wb0.buf = malloc(maxwb0);
        conn->wb0.cap = maxwb0;

        if (!conn->wb0.buf)
                goto err;

        /* write buffer 1 */
        conn->wb1.buf = malloc(maxwb1);
        conn->wb1.cap = maxwb1;

        if (!conn->wb1.buf)
                goto err;

        conn->fd = fd;
        return conn;

err:
        connection_destroy(conn);
        return NULL;
}

void connection_destroy(connection_t *conn)
{
        if (!isbadf(conn->fd)) {
                /* remove in epoll */
                rc_event_del(conn->rc, conn->fd);
                close(conn->fd);
        }

        if (conn->rb.buf)
                free(conn->rb.buf);

        if (conn->wb0.buf)
                free(conn->wb0.buf);

        if (conn->wb1.buf)
                free(conn->wb1.buf);

        free(conn);
}

void connection_write(connection_t *conn, stagbuf_t *wb, const void *data, size_t size)
{
        size_t avail = wb->cap - wb->len;

        if (avail < size)
                goto err;

        memcpy(wb->buf + wb->len, data, size);

        wb->len += size;

        if (!(conn->flags | RC_WRITE)) {
                if (rc_event_mod(conn->rc, conn->fd, RC_READ | RC_WRITE | RC_ET, conn) < 0)
                        goto err;
        }

err:
        connection_destroy(conn);
}