/*
-* SPDX-License-Identifier: MIT
 * Copyright (conn) 2025
 */
#define CONNECTION_EXTERN_FUNC
#include "evlp.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>

#include "utils/log.h"
#include "utils/hashtable.h"
#include "config.h"

struct evlp {
        int epoll_fd;
        int listen_fd;
        int timer_fd;
        on_read_fn_t on_read;
        on_write_fn_t on_write;
        accept_callback_fn_t accept_callback;
        struct hashtable *cqueue;
};

static int _init_timer(evlp_t *evlp)
{
        evlp->timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

        if (evlp->timer_fd < 0) {
                log_error("create evlp timer failed, cause: %s\n", syserr);
                return -1;
        }

        struct itimerspec its = {
                .it_value.tv_sec = 0,
                .it_value.tv_nsec = 1,
                .it_interval.tv_sec = CONNECTION_TIMEOUT_SEC,
                .it_interval.tv_nsec = 0,
        };

        if (timerfd_settime(evlp->timer_fd, 0, &its, NULL) < 0) {
                log_error("timerfd_settime failed, cause: %s\n", syserr);
                goto err;
        }

        struct epoll_event timer_ev = {
                .events = EPOLLIN,
                .data.fd = evlp->timer_fd,
        };

        if (epoll_ctl(evlp->epoll_fd, EPOLL_CTL_ADD, evlp->timer_fd, &timer_ev) < 0) {
                log_error("epoll_ctl add timer_fd failed, cause: %s\n", syserr);
                goto err;
        }

        return 0;

        err:
                close(evlp->timer_fd);
        return -1;
}

static void _evlp_on_timer(evlp_t *evlp)
{
        uint64_t _tv_tmp;
        read(evlp->timer_fd, &_tv_tmp, sizeof(_tv_tmp));

        if (HASHTABLE_IS_EMPTY(evlp->cqueue))
                return;

        time_t now = time(NULL);
        uint64_t arr[evlp->cqueue->size];
        size_t arrsize = 0;

        struct hashtable_iter iter;
        hashtable_iter_init(&iter, evlp->cqueue);

        struct hashtable_iter_ent ent;
        while (hashtable_iter_next(&iter, &ent)) {
                struct connection *conn =
                        (struct connection *) ent.value;

                if ((now - conn->last_active_ts) >
                        conn->idle_timeout_sec || conn->closed) {
                        arr[arrsize++] = (uint64_t) conn->fd;
                }
        }

        for (size_t i = 0; i < arrsize; i++) {
                struct connection *conn =
                        (struct connection *) hashtable_remove(evlp->cqueue, arr[i]);
                connection_destroy(conn);
        }

        if (arrsize > 0)
                log_info("evlp timer destroy %zu connections.\n", arrsize);
}

static void _evlp_on_accept(evlp_t *evlp)
{
        int cli;
        struct host_sockaddr_in addr;
        struct connection *conn;

        while (1) {
                cli = tcp_accept(evlp->listen_fd, &addr);

                if (cli < 0) {
                        RETRY_IF_EINTR();

                        if (is_eagain())
                                break;
                }

                conn = connection_create(cli, &addr);

                if (!conn) {
                        close(cli);
                        continue;
                }

                struct epoll_event ev = {
                        .events = EPOLLIN | EPOLLET,
                        .data.ptr = conn,
                };

                if (epoll_ctl(evlp->epoll_fd, EPOLL_CTL_ADD, cli, &ev) < 0) {
                        log_error("epoll_ctl add client failed: %s\n", syserr);
                        connection_destroy(conn);
                }

                if (evlp->accept_callback)
                        evlp->accept_callback(evlp, conn);

                hashtable_put(evlp->cqueue, (uint64_t) conn->fd, conn);
        }
}

static void _evlp_route_events(evlp_t *evlp,
                               struct epoll_event *events,
                               int nfds)
{
        struct epoll_event *cur_ev;
        struct connection *conn;

        for (int i = 0; i < nfds; i++) {
                cur_ev = &events[i];

                if (cur_ev->events & EPOLLRDHUP
                        || cur_ev->events & EPOLLHUP
                        || cur_ev->events & EPOLLERR) {
                        if (cur_ev->data.ptr) {
                                conn = (struct connection *) cur_ev->data.ptr;
                                epoll_ctl(evlp->epoll_fd, EPOLL_CTL_DEL, conn->fd, NULL);
                                connection_destroy(conn);
                                continue;
                        }
                }

                if (cur_ev->data.fd == evlp->listen_fd) {
                        _evlp_on_accept(evlp);
                        continue;
                }

                if (cur_ev->data.fd == evlp->timer_fd) {
                        _evlp_on_timer(evlp);
                        continue;
                }

                if (evlp->on_read && (cur_ev->events & EPOLLIN))
                        evlp->on_read(evlp, (struct connection *) cur_ev->data.ptr);

                if (evlp->on_write && (cur_ev->events & EPOLLOUT))
                        evlp->on_write(evlp, (struct connection *) cur_ev->data.ptr);
        }
}

evlp_t *evlp_create(int listen_fd, struct evlp_create_info *info)
{
        evlp_t *evlp = malloc(sizeof(evlp_t));

        if (!evlp)
                goto err_null;

        evlp->cqueue = hashtable_create();

        if (!evlp->cqueue)
                goto err_free;

        evlp->on_read = info->on_read;
        evlp->on_write = info->on_write;
        evlp->accept_callback = info->accept_callback;

        evlp->epoll_fd = epoll_create1(EPOLL_CLOEXEC);

        if (evlp->epoll_fd < 0) {
                log_error("create evlp failed, cause: %s\n", syserr);
                goto err_free;
        }

        evlp->listen_fd = listen_fd;

        struct epoll_event tev = {
                .events = EPOLLIN,
                .data.fd = listen_fd,
        };

        if (epoll_ctl(evlp->epoll_fd, EPOLL_CTL_ADD, listen_fd, &tev) < 0) {
                log_error("epoll_ctl add listen_fd failed, cause: %s\n", syserr);
                goto err_close;
        }

        /* timer schedule */
        if (_init_timer(evlp) != 0)
                goto err_close;

        return evlp;

err_close:
        close(evlp->epoll_fd);
err_free:
        free(evlp);
err_null:
        return NULL;
}

void evlp_poll_events(evlp_t *evlp)
{
        int nfds;
        struct epoll_event events[MAX_EVENTS];

        while (1) {
                nfds = epoll_wait(evlp->epoll_fd, events, MAX_EVENTS, -1);

                if (nfds < 0) {
                        log_warn("epoll_wait nfds=%d, err=%s\n", nfds, syserr);
                        continue;
                }

                break;
        }

        _evlp_route_events(evlp, events, nfds);
}

void evlp_mark_writable(evlp_t *evlp, struct connection *conn)
{
        if (!conn->writable) {
                conn->writable = 1;

                struct epoll_event ev = {
                        .events = EPOLLIN | EPOLLOUT | EPOLLET,
                        .data.ptr = conn,
                };

                if (epoll_ctl(evlp->epoll_fd, EPOLL_CTL_MOD, conn->fd, &ev) < 0) {
                        log_error("mark connection %p writable failed: %s\n", conn, syserr);
                        connection_destroy(conn);
                }
        }
}

void evlp_mark_unwritable(evlp_t *evlp, struct connection *conn)
{
        if (conn->writable) {
                conn->writable = 0;

                struct epoll_event ev = {
                        .events = EPOLLIN | EPOLLET,
                        .data.ptr = conn,
                };

                if (epoll_ctl(evlp->epoll_fd, EPOLL_CTL_MOD, conn->fd, &ev) < 0) {
                        log_error("mark connection %p unwritable failed: %s\n", conn, syserr);
                        connection_destroy(conn);
                }
        }
}

void evlp_connection_close(evlp_t *evlp, struct connection *conn)
{
        hashtable_remove(evlp->cqueue, (uint64_t) conn->fd);
        connection_destroy(conn);
}