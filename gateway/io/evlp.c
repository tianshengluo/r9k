/*
-* SPDX-License-Identifier: MIT
 * Copyright (C) 2025 Luo Tiansheng
 */
#define CONNECTION_EXTERN_FUNC
#include "evlp.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include "utils/log.h"
#include "utils/hashtable.h"
#include "config.h"

struct evlp {
        /* core epoll and listening infrastructure */
        int                     epfd;               /* epoll instance */
        int                     listen_fd;          /* listening socket */
        int                     timer_fd;           /* timeout/keepalive timer */

        /* connection management */
        struct hashtable        *actives;           /* active connections hash */
        struct connection       *close_head;        /* deferred close list (lifo) */

        /* callbacks */
        on_accept_fn_t          on_accept;
        on_close_fn_t           on_close;
        on_read_fn_t            on_read;
        on_write_fn_t           on_write;

        /* EMFILE / resource exhaustion handling */
        int                     emfile_guard_fd;
        uint32_t                emfile_conn_cnt;    /* conn count when EMFILE hit */
        int                     reject_new_conn;    /* are we in reject-new-conns mode? */
};

#define CLOSE_HEAD_ADD(_evlp, conn)            \
        (conn)->next = (_evlp)->close_head;    \
        (_evlp)->close_head = (conn)

static void _evlp_hold_emfile_guard(evlp_t *evlp)
{
        evlp->emfile_guard_fd = open("/dev/null", O_RDONLY);
        log_info("evlp hold emfile guard fd: %d\n", evlp->emfile_guard_fd);
}

static void _evlp_release_emfile_guard(evlp_t *evlp)
{
        log_info("evlp release emfile guard fd: %d\n", evlp->emfile_guard_fd);
        close(evlp->emfile_guard_fd);
        evlp->emfile_guard_fd = -1;
}

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
                .it_interval.tv_sec = 5,
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

        if (epoll_ctl(evlp->epfd, EPOLL_CTL_ADD, evlp->timer_fd, &timer_ev) < 0) {
                log_error("epoll_ctl add timer_fd failed, cause: %s\n", syserr);
                goto err;
        }

        return 0;

err:
        close(evlp->timer_fd);
        return -1;
}

static int _evlp_add_listen_sock(evlp_t *evlp, int listen_fd)
{
        evlp->listen_fd = listen_fd;

        struct epoll_event tev = {
                .events = EPOLLIN,
                .data.fd = evlp->listen_fd,
        };
        if (epoll_ctl(evlp->epfd, EPOLL_CTL_ADD, evlp->listen_fd, &tev) < 0) {
                log_error("epoll_ctl add listen_fd failed, cause: %s\n", syserr);
                return -1;
        }

        return 0;
}

static void _evlp_on_timer(evlp_t *evlp)
{
        uint64_t _expired_count;
        read(evlp->timer_fd, &_expired_count, sizeof(_expired_count));

        if (HASHTABLE_IS_EMPTY(evlp->actives))
                return;

        time_t now = time(NULL);
        uint64_t arr[HASHTABLE_SIZE(evlp->actives)];
        size_t arrsize = 0;

        struct hashtable_iter iter;
        hashtable_iter_init(&iter, evlp->actives);

        struct hashtable_iter_ent ent;
        while (hashtable_iter_next(&iter, &ent)) {
                struct connection *conn =
                        (struct connection *) ent.value;

                if ((now - conn->last_active_ts) >
                        conn->idle_timeout_sec) {
                        arr[arrsize++] = (uint64_t) conn->fd;
                }
        }

        for (size_t i = 0; i < arrsize; i++) {
                struct connection *conn =
                        (struct connection *) hashtable_remove(evlp->actives, arr[i]);
                evlp_close(evlp, conn);
        }

        if (arrsize > 0)
                log_info("evlp timer destroy %zu connections.\n", arrsize);
}

static void _evlp_gc(evlp_t *evlp)
{
        struct connection *n;

        if (!evlp->close_head)
                return;

        while ((n = evlp->close_head)) {
                evlp->close_head = n->next;
                connection_destroy(n);
                log_debug("evlp gc destroy %s connection\n", n->addr.sin_addr);
        }
}

static void _evlp_enable_reject(evlp_t *evlp)
{
        evlp->reject_new_conn = 1;
        evlp->emfile_conn_cnt = HASHTABLE_SIZE(evlp->actives);

        epoll_ctl(evlp->epfd,
                  EPOLL_CTL_DEL,
                  evlp->listen_fd,
                  NULL);

        _evlp_release_emfile_guard(evlp);

        log_warn("evlp enable reject mode, emfile connection count: %u\n",
                 evlp->emfile_conn_cnt);
}

static void _evlp_try_disable_reject(evlp_t *evlp)
{
        if (!evlp->reject_new_conn)
                return;

        /* 流量下降 35% 后重新开放客户端连接 */
        if (HASHTABLE_SIZE(evlp->actives) < evlp->emfile_conn_cnt * 65 / 100) {
                evlp->reject_new_conn = 0;

                _evlp_add_listen_sock(evlp, evlp->listen_fd);
                _evlp_hold_emfile_guard(evlp);

                log_warn("evlp disable reject mode, active connection count: %u\n",
                         HASHTABLE_SIZE(evlp->actives));
        }
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

                        if (is_emfile()) {
                                log_error("accept new connection failed %s\n", syserr);
                                _evlp_enable_reject(evlp);
                                continue;
                        }
                }

                if (evlp->reject_new_conn) {
                        close(cli);
                        continue;
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

                if (epoll_ctl(evlp->epfd, EPOLL_CTL_ADD, cli, &ev) < 0) {
                        log_error("epoll_ctl add client failed: %s\n", syserr);
                        connection_destroy(conn);
                }

                if (evlp->on_accept)
                        evlp->on_accept(evlp, conn);

                hashtable_put(evlp->actives, (uint64_t) conn->fd, conn);
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
                                evlp_close(evlp, conn);
                                continue;
                        }
                }

                if (cur_ev->data.fd == evlp->listen_fd && !evlp->reject_new_conn) {
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
        evlp_t *evlp = calloc(1, sizeof(evlp_t));

        if (!evlp)
                goto err_null;

        _evlp_hold_emfile_guard(evlp);

        evlp->actives = hashtable_create();
        evlp->close_head = NULL;

        if (!evlp->actives)
                goto err_free;

        evlp->on_accept = info->on_accept;
        evlp->on_close = info->on_close;
        evlp->on_read = info->on_read;
        evlp->on_write = info->on_write;

        evlp->epfd = epoll_create1(EPOLL_CLOEXEC);

        if (evlp->epfd < 0) {
                log_error("create evlp failed, cause: %s\n", syserr);
                goto err_free;
        }

        _evlp_add_listen_sock(evlp, listen_fd);

        /* timer schedule */
        if (_init_timer(evlp) != 0)
                goto err_close;

        return evlp;

err_close:
        close(evlp->epfd);
err_free:
        free(evlp);
err_null:
        return NULL;
}

void evlp_destroy(evlp_t *evlp)
{
        if (!evlp)
                return;

        close(evlp->emfile_guard_fd);
        close(evlp->timer_fd);
        close(evlp->listen_fd);

        struct hashtable_iter iter;
        hashtable_iter_init(&iter, evlp->actives);

        struct hashtable_iter_ent ent;
        while (hashtable_iter_next(&iter, &ent))
                evlp_close(evlp, (struct connection *) ent.value);

        _evlp_gc(evlp);

        hashtable_destroy(evlp->actives);
        close(evlp->epfd);
}

void evlp_poll_events(evlp_t *evlp)
{
        int nfds;
        struct epoll_event events[MAX_EVT];

        while (1) {
                nfds = epoll_wait(evlp->epfd, events, MAX_EVT, -1);

                if (nfds < 0) {
                        if (!is_eagain())
                                log_error("epoll_wait nfds=%d, err=%s\n", nfds, syserr);
                        continue;
                }

                break;
        }

        _evlp_route_events(evlp, events, nfds);
        _evlp_gc(evlp);
        _evlp_try_disable_reject(evlp);
}

void evlp_enable_write(evlp_t *evlp, struct connection *conn)
{
        if (!conn->writable) {
                conn->writable = 1;

                struct epoll_event ev = {
                        .events = EPOLLIN | EPOLLOUT | EPOLLET,
                        .data.ptr = conn,
                };

                if (epoll_ctl(evlp->epfd, EPOLL_CTL_MOD, conn->fd, &ev) < 0) {
                        log_error("enable connection %p writable failed: %s\n", conn, syserr);
                        connection_destroy(conn);
                }
        }
}

void evlp_disable_write(evlp_t *evlp, struct connection *conn)
{
        if (conn->writable) {
                conn->writable = 0;

                struct epoll_event ev = {
                        .events = EPOLLIN | EPOLLET,
                        .data.ptr = conn,
                };

                if (epoll_ctl(evlp->epfd, EPOLL_CTL_MOD, conn->fd, &ev) < 0) {
                        log_error("disable connection %p write failed: %s\n", conn, syserr);
                        evlp_close(evlp, conn);
                }
        }
}

void evlp_close(evlp_t *evlp, struct connection *conn)
{
        if (conn->state == CONN_STATE_CLOSING)
                return;

        conn->state = CONN_STATE_CLOSING;

        hashtable_remove(evlp->actives, (uint64_t) conn->fd);

        epoll_ctl(evlp->epfd,
                  EPOLL_CTL_DEL,
                  conn->fd,
                  NULL);

        CLOSE_HEAD_ADD(evlp, conn);

        if (evlp->on_close)
                evlp->on_close(evlp, conn);
}