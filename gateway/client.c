/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Varketh Nockrath
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <r9k/readline.h>
#include <r9k/yyjson.h>

#include "io/socket.h"
#include "config.h"
#include "utils/log.h"
#include "ipc.h"

static size_t off = 0;
static char buf[64 * 1024];

#define MAX_FDS 5
int fds[MAX_FDS];

static void writebuf(const char *message)
{
        ipc_t ipc;
        int n;
        char body[32 * 1024];

        n = snprintf(body, sizeof(body), "{"
                                           "\"msg_id\":1023890128390189321,"
                                           "\"msg_at\":[10012173,20038192,30041002],"
                                           "\"from\":10051182,"
                                           "\"to\":\"G100239301932\","
                                           "\"is_reply\":true,"
                                           "\"reply_to_msg_id\":1023890128390189320,"
                                           "\"timestamp\":1770345129543,"
                                           "\"msg_content"
                                           "\":\"%s\""
                                         "}", message);
        ipc_header_serialize(&ipc, n);

        memcpy(buf + off, &ipc, sizeof(ipc_t));
        off += sizeof(ipc_t);

        memcpy(buf + off, body, n);
        off += n;

        printf("%lu\n", n + sizeof(ipc_t));
}

void client_start()
{
        srand(time(NULL));

        int fd;

        for (int i = 0; i < MAX_FDS; i++) {
                fd = tcp_connect("127.0.0.1", GW_SERVER_PORT);

                if (fd < 0) {
                        log_error("connect to 127.0.0.1 %d failed, cause: %s\n", GW_SERVER_PORT, syserr);
                        continue;
                }

                fds[i] = fd;
        }

        char prompt[64];
        char *line = NULL;

        while (1) {
                off = 0;
                fd = fds[rand() % MAX_FDS + 1];

                snprintf(prompt, sizeof(prompt), "%d > ", fd);

                line = readline(prompt);

                if (!line)
                        continue;

                writebuf(line);
                send(fd, &buf, off, MSG_NOSIGNAL);
        }

}