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

#define MAX_CLIENT 128

static int randc(void)
{
        int r;
        int limit = RAND_MAX - (RAND_MAX % MAX_CLIENT);

        do {
                r = rand();
        } while (r >= limit);

        return r % MAX_CLIENT;
}

static void client_exit(int fd)
{
        close(fd);
        exit(0);
}

__attr_noreturn
int client_start(int argc, char **argv)
{
        __attr_ignore2(argc, argv);

        int   fds[MAX_CLIENT];

        char *line = NULL;
        struct icp ic;
        ssize_t n;

        for (int i = 0; i < MAX_CLIENT; i++) {
                fds[i] = socket_connect("127.0.0.1", 26214);
                if (fds[i] < 0)
                        exit(1);
        }

        clear();

        while (1) {
                int fd = fds[randc()];
                printf("use fd: %d\n", fd);

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