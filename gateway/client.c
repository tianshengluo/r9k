/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Varketh Nockrath
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <r9k/readline.h>

#include "io/socket.h"
#include "config.h"
#include "eim.h"
#include "log.h"

void client_start()
{
        int fd;

        fd = tcp_connect("127.0.0.1", PORT);

        if (fd < 0) {
                logger_error("connect to 127.0.0.1 %d failed, cause: %s\n", PORT, syserr);
                exit(1);
        }

        char prompt[64];
        char *line = NULL;
        eim_t eim;

        while (1) {
                snprintf(prompt, sizeof(prompt), "%d > ", fd);

                line = readline(prompt);

                if (!line)
                        continue;

                eimb(line, &eim);

                send(fd, &eim, EIM_SIZE, 0);
                sleep(5);
                send(fd, line, strlen(line), 0);
        }

}