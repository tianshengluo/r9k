/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/event.h>
#include <unistd.h>
#include <errno.h>

#include <r9k/panic.h>

#include "socket.h"

#define MAX_CONNECT 65535
#define PORT        888

#define PROTO_MAGIC 0xBADCAB1E

struct proto {
        uint32_t magic;
        uint32_t version;
        uint32_t flags;

        uint64_t msg_id;
        uint64_t session_id;
        uint64_t sender_id;
        uint64_t receiver_id;

        uint16_t msg_type;
        uint32_t body_len;
} __attribute__((packed));

int ev_poll_wait(int kq, struct kevent *events, size_t max_events)
{
        int nev;

        for (;;) {
                nev = kevent(kq, NULL, 0, events, max_events, NULL);
                if (nev > 0)
                        break;

                if (errno == EINTR)
                        continue;

                perror("kpoll_wait() failed");
                return -1;
        }

        return nev;
}

void ev_poll_add(int listen_fd)
{
        int cli;
        struct kevent change;
        sockconn_t *conn;

        while ((cli = socket_accept(listen_fd, NULL)) >= 0) {
                conn = socket_conn_alloc(sizeof(struct proto));
                if (!conn) {
                        close(cli);
                        return;
                }

                EV_SET(&change, cli, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, conn);
                if (kevent(cli, &change, 1, NULL, 0, NULL) == -1) {
                        perror("kevent() failed");
                        close(cli);
                        socket_conn_free(conn);
                        return;
                }
        }
}

void handle_client(sockconn_t *conn)
{

}

void handle_events(struct kevent *events, int nevents, int listen_fd)
{
        int fd;
        struct kevent *ev;

        for (int i = 0; i < nevents; i++) {
                ev = &events[i];
                fd = (int) ev->ident;

                if (fd == listen_fd) {
                        ev_poll_add(fd);
                        continue;
                }

                handle_client((sockconn_t *) ev->udata);
        }
}

int main(int argc, char **argv)
{
        __attr_ignore2(argc, argv);

        int listen_fd;
        int kq;
        struct kevent change;
        struct kevent events[MAX_CONNECT];

        listen_fd = socket_start(PORT, MAX_CONNECT);
        if (listen_fd < 0) {
                perror("socket_start() failed");
                return -1;
        }

        kq = kqueue();
        if (kq == -1) {
                perror("kqueue() failed");
                goto err;
        }

        EV_SET(&change, listen_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
        if (kevent(kq, &change, 1, NULL, 0, NULL) == -1) {
                perror("kevent() register");
                goto err;
        }

        for (;;) {
                int nev = ev_poll_wait(kq, events, MAX_CONNECT);
                if (nev < 0)
                        goto err;

                handle_events(events, nev, listen_fd);
        }

err:
        close(listen_fd);

        if (kq >= 0)
                close(kq);

        return -1;
}

