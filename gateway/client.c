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
#include "utils/log.h"
#include "ipc.h"

void client_start()
{
        int fd;
        ipc_hdr_t hdr;

        fd = tcp_connect("127.0.0.1", PORT);

        if (fd < 0) {
                log_error("connect to 127.0.0.1 %d failed, cause: %s\n", PORT, syserr);
                exit(1);
        }

        char prompt[64];
        char *line = NULL;

        while (1) {
                snprintf(prompt, sizeof(prompt), "%d > ", fd);

                line = readline(prompt);

                if (!line)
                        continue;

                size_t len = strlen(line);
                ipc_hdr_build(&hdr, 123, 987, (uint32_t) len);

                send(fd, &hdr, sizeof(ipc_hdr_t), 0);
                send(fd, line, len, 0);
        }

}