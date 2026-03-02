/*
-* SPDX-License-Identifier: MIT
 * Copyright (C) 2025 Luo Tiansheng
 */
#ifndef SOCKET_H_
#define SOCKET_H_

#include <arpa/inet.h>
#include <errno.h>

#define RETRY_IF_EINTR()        \
        if (errno == EINTR)     \
                continue

#define is_eagain() (errno == EAGAIN || errno == EWOULDBLOCK)
#define is_emfile() (errno == EMFILE || errno == ENFILE)

struct host_sockaddr_in {
        uint16_t sin_port;
        char     sin_addr[INET6_ADDRSTRLEN];
};

int tcp_create_listener(int port);
int udp_create_bound_socket(int port);
int tcp_connect(const char *host, int port);
int tcp_accept(int fd, struct host_sockaddr_in *addr);

#endif /* SOCKET_H_ */