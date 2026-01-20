/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <r9k/string.h>
#include <r9k/readline.h>
#include <r9k/compiler_attrs.h>

#include "icp.h"
#include "socket.h"

static void client_exit(int fd)
{
        close(fd);
        exit(0);
}

__attr_noreturn
int client_start(int argc, char **argv)
{
        __attr_ignore2(argc, argv);

        int   fd;
        char *line = NULL;
        struct icp ic;
        ssize_t n;

        fd = socket_connect("127.0.0.1", 26214);
        if (fd < 0)
                exit(1);

        clear();

        while (1) {
                line = readline("gateway > ");
                if (line == NULL)
                        continue;

                add_history(line);

                if (streq(line, "clear")) {
                        clear();
                        continue;
                }

                if (streq(line, "exit"))
                        client_exit(fd);

                icp_build(&ic, line);

                n = send(fd, &ic, sizeof(ic), 0);
                if (n == 0)
                        client_exit(fd);

                n = send(fd, line, ic.paksiz, 0);
                if (n == 0)
                        client_exit(fd);

                free(line);
        }
}