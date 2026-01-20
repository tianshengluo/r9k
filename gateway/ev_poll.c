/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#include "ev_poll.h"
#include <stdio.h>

int ev_add(int epfd, int fd, uint32_t events, void *ptr)
{
        struct epoll_event ev;

        ev.events = events;
        ev.data.ptr = ptr;

        if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
                perror("epoll_ctl ADD");
                return -1;
        }

        return 0;
}