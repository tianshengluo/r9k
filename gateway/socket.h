/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#ifndef SOCKET_H_
#define SOCKET_H_

#include <arpa/inet.h>
#include <errno.h>

#ifndef __BYTE_ORDER
#include <endian.h>
#endif /* __BYTE_ORDER */

#ifndef ntohll
static inline uint64_t ntohll (uint64_t __netlong)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
        uint64_t low_part  = ntohl((uint32_t)(__netlong & 0xFFFFFFFFULL));
        uint64_t high_part = ntohl((uint32_t)((__netlong >> 32) & 0xFFFFFFFFULL));
        return (high_part << 32) | low_part;
#else
        return __netlong;
#endif /* __BYTE_ORDER */
}
#endif /* ntohll */

#define RETRY_ONCE_ENTR()   \
        if (errno == EINTR) \
                continue

#define is_eagain() (errno == EAGAIN || errno == EWOULDBLOCK)

int tcp_create_listener(int port);
int udp_create_bound_socket(int port);
int tcp_connect(const char *host, int port);
int tcp_accept(int fd, struct sockaddr_in *addr);
int isbadf(int fd);
int set_nonblock(int fd);

#endif /* SOCKET_H_ */