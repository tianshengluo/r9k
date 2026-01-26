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

int tcp_create_listener(int port)
{
        int fd, opt = 1;
        struct sockaddr_in addr;

        fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        if (fd < 0)
                return -1;

        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port);

        if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
                goto err;

        if (listen(fd, SOMAXCONN) < 0)
                goto err;

        return fd;

err:
        close(fd);
        return -1;
}


int udp_create_bound_socket(int port)
{
        int fd, opt = 1;
        struct sockaddr_in addr;

        fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        if (fd < 0)
                return -1;

        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port);

        if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
                close(fd);
                return -1;
        }

        return fd;
}

int tcp_connect(const char *host, int port)
{
        int fd;
        struct sockaddr_in addr;

        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
                perror("socket() failed");
                return -1;
        }

        memset(&addr, 0, sizeof(struct sockaddr_in));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(host);
        addr.sin_port = htons(port);

        if (connect(fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) < 0) {
                perror("connect() failed");
                close(fd);
                return -1;
        }

        return fd;
}

int tcp_accept(int fd, struct sockaddr_in *addr)
{
        int cli;

        while (1) {
                if (addr) {
                        socklen_t len = sizeof(struct sockaddr_in);

                        cli = accept(fd,
                                     (struct sockaddr *) addr,
                                     &len);
                } else {
                        cli = accept(fd, NULL, NULL);
                }

                if (cli >= 0) {
                        if (set_nonblock(cli) < 0) {
                                close(cli);
                                return -1;
                        }
                        return cli;
                }

                RETRY_ONCE_ENTR();

                if (errno == EAGAIN || errno == EWOULDBLOCK)
                        return -1;

                perror("socket_accept() failed");
                return -1;
        }
}

int isbadf(int fd)
{
        if (fcntl(fd, F_GETFD, 0) < 0)
                if (errno == EBADF)
                        return 1;
        return 0;
}

int set_nonblock(int fd)
{
        int flags = fcntl(fd, F_GETFL, 0);

        if (flags < 0)
                return -1;

        return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}