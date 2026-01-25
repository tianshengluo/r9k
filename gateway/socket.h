/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#ifndef SOCKET_H_
#define SOCKET_H_

#include <arpa/inet.h>
#include <errno.h>

#define RETRY_ONCE_ENTR()   \
        if (errno == EINTR) \
                continue

#define is_eagain() (errno == EAGAIN || errno == EWOULDBLOCK)

int tcp_create_listener(int port);
int udp_create_bound_socket(int port);
int tcp_connecct(const char *host, int port);
int tcp_accept(int fd, struct sockaddr_in *addr);
int set_nonblock(int fd);

#endif /* SOCKET_H_ */