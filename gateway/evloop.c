/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 *
 * Enterprise Instant Messaging.
 */
#include "evloop.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/resource.h>

#include "io/socket.h"

struct evloop {
        int event_fd;
        int listen_fd;
        int max_events;
        uint32_t max_fds;
        struct evloop_event **evmap;
};

static uint32_t to_epoll_events(uint32_t events)
{
        uint32_t ep_events = 0;

        if (events & EVLOOP_READ)
                ep_events |= EPOLLIN;

        if (events & EVLOOP_WRITE)
                ep_events |= EPOLLOUT;

        if (events & EVLOOP_EDGE)
                ep_events |= EPOLLET;

        return ep_events | EPOLLHUP | EPOLLRDHUP | EPOLLERR;
}

static uint32_t to_evloop_events(uint32_t events)
{
        uint32_t evl_events = 0;

        if (events & EPOLLIN)
                evl_events |= EVLOOP_READ;

        if (events & EPOLLOUT)
                evl_events |= EVLOOP_WRITE;

        if (events & EPOLLET)
                evl_events |= EVLOOP_EDGE;

        if (events & (EPOLLHUP | EPOLLRDHUP))
                evl_events |= EVLOOP_EOF;

        if (events & EPOLLERR)
                evl_events |= EVLOOP_ERROR;

        return evl_events;
}

static struct evloop_event *evldup(struct evloop_event *ev)
{
        struct evloop_event *r;

        r = malloc(sizeof(struct evloop_event));

        if (!r)
                return NULL;

        memcpy(r, ev, sizeof(struct evloop_event));

        return r;
}

evloop_t *evloop_create(int listen_fd)
{
        evloop_t *evl;

        evl = calloc(1, sizeof(evloop_t));

        if (!evl)
                return NULL;

        evl->listen_fd = listen_fd;
        evl->max_events = 1024;

        evl->event_fd = epoll_create1(EPOLL_CLOEXEC);

        if (evl->event_fd < 0)
                goto err;

        /* init evmap */
        struct rlimit rl;
        if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
                goto err;

        evl->max_fds = rl.rlim_max;
        evl->evmap = malloc(evl->max_fds * sizeof(struct evloop_event *));

        if (!evl->evmap)
                goto err;

        /* add listen fd to evl */
        struct evloop_event ev = {
                .events = EVLOOP_READ,
                .fd = evl->listen_fd,
        };

        if (evloop_ctl(evl, EVCTL_ADD, &ev) < 0)
                goto err;
        
        return evl;

err:
        evloop_close(evl);
        return NULL;
}

void evloop_close(evloop_t *evl)
{
        if (!evl)
                return;

        if (!isbadf(evl->event_fd))
                close(evl->event_fd);

        if (evl->evmap) {
                struct evloop_event *evt;

                for (uint32_t i = 0; i < evl->max_fds; i++) {
                        evt = evl->evmap[i];

                        if (!evt)
                                continue;

                        if (evt->fd >= 0)
                                close(evt->fd);

                        free(evt);
                }

                free(evl->evmap);
        }

        free(evl);
}

int evloop_ctl(evloop_t *evl, int ctl, struct evloop_event *ev)
{
        struct epoll_event epoll_ev;
        struct evloop_event *stored;

        if (ctl == EVCTL_ADD) {
                stored = evldup(ev);

                if (!stored)
                        return -errno;

                epoll_ev.events = to_epoll_events(ev->events);
                epoll_ev.data.ptr = stored;

                evl->evmap[ev->fd] = stored;

                return epoll_ctl(evl->event_fd, EPOLL_CTL_ADD, ev->fd, &epoll_ev);
        }

        if (ctl == EVCTL_MOD) {
                epoll_ev.events = to_epoll_events(ev->events);
                epoll_ev.data.ptr = evl->evmap[ev->fd];
                return epoll_ctl(evl->event_fd, EPOLL_CTL_MOD, ev->fd, &epoll_ev);
        }

        if (ctl == EVCTL_DEL) {
                stored = evl->evmap[ev->fd];

                evl->evmap[ev->fd] = NULL;

                int r = epoll_ctl(evl->event_fd, EPOLL_CTL_DEL, stored->fd, NULL);

                free(stored);

                return r;
        }

        return -EINVAL;
}

int evloop_wait(evloop_t *evl, struct evloop_event *evs, int max_events)
{
        struct evloop_event *evl_event;
        struct epoll_event events[max_events];

        while (1) {
                int nfds = epoll_wait(evl->event_fd, events, max_events, -1);

                if (nfds > 0) {
                        for (int i = 0; i < nfds; i++) {
                                evl_event = (struct evloop_event *) events[i].data.ptr;
                                evs[i].events = to_evloop_events(events[i].events);
                                evs[i].fd = evl_event->fd;
                                evs[i].ptr = evl_event->ptr;
                        }
                        return nfds;
                }

                if (nfds < 0) {
                        if (errno == EINTR)
                                return 0;
                        return -errno;
                }
        }
}