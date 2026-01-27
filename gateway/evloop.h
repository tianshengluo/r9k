/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 *
 * Enterprise Instant Messaging.
 */
#ifndef EVLOOP_H_
#define EVLOOP_H_

#include <stdint.h>

#define EVLOOP_READ  0x0001
#define EVLOOP_WRITE 0x0002
#define EVLOOP_EDGE  0x0004 /* epoll: ET, kqueue: EV_CLEAR */
#define EVLOOP_EOF   0x0008 /* epoll: HUP or RDHUP, kqueue: EOF */
#define EVLOOP_ERROR 0x0010

#define EVCTL_ADD    1
#define EVCTL_DEL    2
#define EVCTL_MOD    3

typedef struct evloop evloop_t;

struct evloop_event {
        int      fd;
        uint32_t events;
        void*    ptr;
};

evloop_t *evloop_create(int listen_fd);
void evloop_close(evloop_t *evl);
int evloop_ctl(evloop_t *evl, int ctl, struct evloop_event *ev);
int evloop_wait(evloop_t *evl, struct evloop_event *evs, int max_events);

#endif /* EVLOOP_H_ */