/*
-* SPDX-License-Identifier: MIT
 * Copyright (conn) 2025
 */
#include "evlp.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>

#include "utils/log.h"
#include "utils/hashtable.h"
#include "config.h"

struct evlp {
        int k_fd;
        int listen_fd;
        on_read_fn_t on_read;
        on_write_fn_t on_write;
        accept_callback_fn_t accept_callback;
        struct hashtable *connections;
};

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

                if (epoll_ctl(evlp->k_fd, EPOLL_CTL_ADD, cli, &ev) < 0) {
                        log_error("epoll_ctl add client failed: %s\n", syserr);
                        connection_destroy(conn);
                }

                if (evlp->accept_callback)
                        evlp->accept_callback(evlp, conn);

                hashtable_put(evlp->connections, (uint64_t) conn->fd, conn);
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
                                epoll_ctl(evlp->k_fd, EPOLL_CTL_DEL, conn->fd, NULL);
                                connection_destroy(conn);
                                continue;
                        }
                }

                if (cur_ev->data.fd == evlp->listen_fd) {
                        _evlp_on_accept(evlp);
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

        evlp->connections = hashtable_create();

        if (!evlp->connections)
                goto err_free;

        evlp->on_read = info->on_read;
        evlp->on_write = info->on_write;
        evlp->accept_callback = info->accept_callback;

        evlp->k_fd = epoll_create1(EPOLL_CLOEXEC);

        if (evlp->k_fd < 0) {
                log_error("create evlp failed, cause: %s\n", syserr);
                goto err_free;
        }

        evlp->listen_fd = listen_fd;

        struct epoll_event tev = {
                .events = EPOLLIN,
                .data.fd = listen_fd,
        };

        if (epoll_ctl(evlp->k_fd, EPOLL_CTL_ADD, listen_fd, &tev) < 0) {
                log_error("epoll_ctl add listen_fd failed: %s\n", syserr);
                goto err_close;
        }

        return evlp;

err_close:
        close(evlp->k_fd);
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
                nfds = epoll_wait(evlp->k_fd, events, MAX_EVENTS, -1);

                if (nfds < 0) {
                        log_warn("epoll_wait: nfds=%d, err=%s\n", nfds, syserr);
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

                if (epoll_ctl(evlp->k_fd, EPOLL_CTL_MOD, conn->fd, &ev) < 0) {
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

                if (epoll_ctl(evlp->k_fd, EPOLL_CTL_MOD, conn->fd, &ev) < 0) {
                        log_error("mark connection %p unwritable failed: %s\n", conn, syserr);
                        connection_destroy(conn);
                }
        }
}
