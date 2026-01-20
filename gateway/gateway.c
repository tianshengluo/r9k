/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#include <string.h>
#include <unistd.h>
#include <sys/event.h>
#include <r9k/panic.h>

#include "icp.h"
#include "socket.h"
#include "client.h"

#define PORT       26214
#define MAX_EVENTS 65535
#define MAX_ACCPET 128 /* accept max once. */

struct gateway_ctx {
        int listen_fd;
        int kq;
        int nev;
        struct kevent events[MAX_EVENTS];
};

struct gwsockbind {
        int    fd;
        size_t cap;
        size_t len;
        char  *stagbuf;
};

static struct gwsockbind *gw_sock_bind(int fd)
{
        struct gwsockbind *bind;

        bind = calloc(1, sizeof(struct gwsockbind));
        if (!bind)
                return NULL;

        bind->fd  = fd;
        bind->cap = MAX_STAGING;

        bind->stagbuf = malloc(bind->cap);
        if (!bind->stagbuf) {
                free(bind);
                return NULL;
        }

        return bind;
}

static void gw_sock_close(struct gwsockbind *bind)
{
        if (!bind)
                return;

        close(bind->fd);
        free(bind->stagbuf);
        free(bind);
}

static int init_gateway_ctx(struct gateway_ctx *ctx)
{
        struct kevent change;

        ctx->listen_fd = socket_start(PORT);
        if (ctx->listen_fd < 0) {
                perror("socket_start(%d) failed");
                return -1;
        }

        set_nonblock(ctx->listen_fd);

        ctx->kq = kqueue();
        if (ctx->kq < 0) {
                close(ctx->listen_fd);
                perror("kqueue() failed");
                return -1;
        }

        EV_SET(&change, ctx->listen_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
        if (kevent(ctx->kq, &change, 1, NULL, 0, NULL) < 0) {
                perror("kevent() failed");
                close(ctx->listen_fd);
                close(ctx->kq);
                return -1;
        }

        printf("gateway server start listening in %d...\n", PORT);

        return 0;
}

static int gw_poll_events(struct gateway_ctx *ctx)
{
        ctx->nev = kevent(ctx->kq, 0, 0, ctx->events, MAX_EVENTS, NULL);
        if (ctx->nev < 0)
                return -1;

        return ctx->nev;
}

/* listen_fd trigger always by LT mode */
static void gw_event_add(struct gateway_ctx *ctx)
{
        int cli;
        struct gwsockbind *bind;
        struct kevent change;

        for (int i = 0; i < MAX_ACCPET; i++) {
                cli = socket_accept(ctx->listen_fd, NULL);

                if (cli < 0) {
                        if (is_eagain())
                                break;
                }

                set_nonblock(cli);
                bind = gw_sock_bind(cli);

                EV_SET(&change, cli, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, bind);
                if (kevent(ctx->kq, &change, 1, NULL, 0, NULL) < 0) {
                        perror("kevent(EV_ADD) failed");
                        gw_sock_close(bind);
                        continue;
                }
        }
}

static void gw_event_read(struct gateway_ctx *ctx, struct gwsockbind *bind)
{
        __attr_ignore(ctx);

        ssize_t n;

        while (1) {
                n = recv(bind->fd,
                         bind->stagbuf + bind->len,
                         bind->cap - bind->len,
                         0);

                if (n == 0)
                        goto sock_err;

                if (n < 0) {
                        RETRY_ONCE_ENTR();

                        if (is_eagain())
                                return;
                }

                bind->len += n;
                printf("len: %zu\n", bind->len);

                while (bind->len >= ICP_SIZE) {
                        switch (icp_packet(bind->stagbuf, &bind->len)) {
                                case ICP_ERROR_BAD_MAGIC:
                                case ICP_ERROR_BAD_LENGTH:
                                case ICP_ERROR_BAD_VERSION:
                                        goto sock_err;
                                case ICP_ERROR_INCOMPLETE:
                                        break;
                                default:
                                        continue;
                        }
                        break;
                }
        }

sock_err:
        gw_sock_close(bind);
}

static void gw_event_loop(struct gateway_ctx *ctx)
{
        int fd;
        struct kevent *event;

        if (ctx->nev <= 0)
                return;

        for (int i = 0; i < ctx->nev; i++) {
                event = &ctx->events[i];
                fd = (int) event->ident;

                if (fd == ctx->listen_fd) {
                        gw_event_add(ctx);
                        continue;
                }

                /* read */
                gw_event_read(ctx, (struct gwsockbind *) event->udata);
        }
}

__attr_noreturn
int main(int argc, char **argv)
{
        __attr_ignore2(argc, argv);

        if (argc > 1 && strcmp(argv[1], "-c") == 0)
                client_start(argc, argv);

        struct gateway_ctx ctx;
        memset(&ctx, 0, sizeof(struct gateway_ctx));

        if (init_gateway_ctx(&ctx) != 0)
                exit(1);

        while (1) {
                if (gw_poll_events(&ctx) < 0)
                        continue;

                gw_event_loop(&ctx);
        }
}

