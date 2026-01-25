/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <r9k/compiler_attrs.h>

#include "socket.h"

#define PORT 8139

__attr_noreturn
int main(int argc, char **argv)
{
        __attr_ignore2(argc, argv);

        int fd;

        fd = tcp_create_listener(PORT);

        if (fd < 0)
                exit(1);

        printf("[%-6s] listening on port %d\n", "sock", PORT);

        close(fd);
        printf("[%-6s] server socket close fd %d\n", "sock", fd);

        return 0;
}
