/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#ifndef EV_POLL_H_
#define EV_POLL_H_

#include <sys/epoll.h>

int ev_add(int epfd, int fd, uint32_t events, void *ptr);

#endif /* EV_POLL_H_ */
