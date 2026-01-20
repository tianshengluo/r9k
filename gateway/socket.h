/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#ifndef SOCKET_H_
#define SOCKET_H_

#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <errno.h>

#define RETRY_ONCE_ENTR()   \
        if (errno == EINTR) \
                continue

#define is_eagain() (errno == EAGAIN || errno == EWOULDBLOCK)

int socket_start(int port);
int socket_connect(const char *host, int port);
int socket_accept(int fd, struct sockaddr_in *addr);
int set_nonblock(int fd);

#endif /* SOCKET_H_ */