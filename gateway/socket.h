/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#ifndef SOCKET_H_
#define SOCKET_H_

#include <arpa/inet.h>

#define SOCK_SERVER 0
#define SOCK_CLIENT 1

typedef enum
{
        SOCK_ERR_CREATE = -1,
        SOCK_ERR_BIND,
        SOCK_ERR_LISTEN,
        SOCK_ERR_ACCEPT,
} sockerr_t;

typedef struct {
        int    fd;
        size_t size;
        size_t off;
        char  *stagbuf;
} sockconn_t;

int socket_start(int port, int max_listen);
int socket_accept(int fd, struct sockaddr_in *addr);

ssize_t socket_read(int fd, void *ptr, size_t size, int flags);
ssize_t socket_write(int fd, const void *ptr, size_t size, int flags);

int set_nonblock(int fd);

sockconn_t *socket_conn_alloc(size_t stagbuf_size);
void socket_conn_free(sockconn_t *conn);

#endif /* SOCKET_H_ */