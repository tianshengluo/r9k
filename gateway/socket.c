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

int socket_start(int port)
{
        int fd;
        int opt = 1;
        struct sockaddr_in addr;

        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
                perror("socket create failed");
                return -1;
        }

        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        memset(&addr, 0, sizeof(struct sockaddr_in));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port);

        if (bind(fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) < 0) {
                perror("bind failed");
                close(fd);
                return -1;
        }

        if (listen(fd, 65535) < 0) {
                perror("listen failed");
                close(fd);
                return -1;
        }

        return fd;
}

int socket_accept(int fd, struct sockaddr_in *addr)
{
        int cli;

        while (1) {
                if (addr) {
                        socklen_t len = sizeof(struct sockaddr_in);

                        cli = accept(fd,
                                     (struct sockaddr *) &addr,
                                     &len);
                } else {
                        cli = accept(fd, NULL, NULL);
                }

                if (cli >= 0)
                        return cli;

                RETRY_ONCE_ENTR();

                if (errno == EAGAIN || errno == EWOULDBLOCK)
                        return -1;

                perror("socket_accept() failed");
                return -1;
        }
}

int set_nonblock(int fd)
{
        int flags = fcntl(fd, F_GETFL, 0);

        if (flags < 0)
                return -1;

        return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}