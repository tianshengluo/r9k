/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#include "socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

int socket_start(int port, int max_listen)
{
        int fd;
        int opt = 1;
        struct sockaddr_in addr;

        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
                perror("socket create failed");
                return SOCK_ERR_CREATE;
        }

        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        memset(&addr, 0, sizeof(struct sockaddr_in));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port);

        if (bind(fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) < 0) {
                perror("bind failed");
                close(fd);
                return SOCK_ERR_BIND;
        }

        if (listen(fd, max_listen) < 0) {
                perror("listen failed");
                close(fd);
                return SOCK_ERR_LISTEN;
        }

        return fd;
}

int socket_accept(int fd, struct sockaddr_in *addr)
{
        int cli;

        for (;;) {
                if (addr) {
                        socklen_t len = sizeof(addr);

                        cli = accept(fd,
                                     (struct sockaddr *) addr,
                                     &len);
                } else {
                        cli = accept(fd, NULL, NULL);
                }

                if (cli >= 0)
                        return cli;

                if (errno == EINTR)
                        continue;

                return -1;
        }
}

ssize_t socket_read(int fd, void *ptr, size_t size, int flags)
{
        size_t off = 0;

        while (off < size) {
                ssize_t n = recv(fd, (char *) ptr + off, size - off, flags);

                if (n == 0)
                        return off ? (ssize_t) off : 0;

                if (n < 0) {
                        if (errno == EINTR)
                                continue;
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                                return off ? (ssize_t) off : 0;
                        return -1;
                }

                off += n;
        }

        return (ssize_t) off;
}

ssize_t socket_write(int fd, const void *ptr, size_t size, int flags)
{
        size_t off = 0;

        while (off < size) {
                ssize_t n = send(fd, (char *) ptr + off, size - off, flags);

                if (n <= 0) {
                        if (errno == EINTR)
                                continue;
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                                return off ? (ssize_t) off : 0;
                        return -1;
                }

                off += n;
        }

        return (ssize_t) off;
}

int set_nonblock(int fd)
{
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags < 0)
                return -1;
        return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

sockconn_t *socket_conn_alloc(size_t stagbuf_size)
{
        sockconn_t *conn;

        conn = calloc(1, sizeof(conn));
        if (!conn)
                return NULL;

        conn->stagbuf = calloc(1, stagbuf_size);
        if (!conn->stagbuf) {
                free(conn);
                return NULL;
        }

        conn->size = stagbuf_size;

        return conn;
}

void socket_conn_free(sockconn_t *conn)
{
        if (conn) {
                free(conn->stagbuf);
                free(conn);
        }
}