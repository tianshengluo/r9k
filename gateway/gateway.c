/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#include "gateway.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <time.h>
#include <r9k/string.h>
#include <r9k/panic.h>

#include "socket.h"
#include "icp.h"
#include "ev_poll.h"

struct connection {
        int      fd;
        time_t   heartbeat;
        uint32_t cap;
        uint32_t len;
        char    *stagbuf;
};

struct gateway {
        int listen_fd;
        int epfd;
        struct config *cnf;
        struct connection *conns;
};

static void gateway_init(struct gateway *ctx, struct config *cnf)
{
        memset(ctx, 0, sizeof(struct gateway));

        ctx->listen_fd = -1;
        ctx->epfd  = -1;
        ctx->cnf = cnf;

        ctx->conns = calloc(cnf->limit, sizeof(struct connection *));

        if (!ctx->conns) {
                perror("calloc");
                exit(1);
        }
}

static void gateway_destroy(struct gateway *ctx)
{
        if (ctx->listen_fd >= 0)
                close(ctx->listen_fd);

        if (ctx->epfd >= 0)
                close(ctx->epfd);

        if (ctx->conns)
                free(ctx->conns);
}

static struct connection *connection_create(int fd)
{
        struct connection *conn;

        conn = calloc(1, sizeof(struct connection));

        if (!conn)
                return NULL;

        conn->fd        = fd;
        conn->heartbeat = time(NULL);
        conn->cap       = MAX_STAGING;
        conn->stagbuf   = malloc(MAX_STAGING);

        if (!conn->stagbuf) {
                free(conn);
                return NULL;
        }

        return conn;
}

static void connection_destroy(struct connection *conn)
{

}

static void gateway_new_connection(struct gateway *ctx, int fd)
{
        int cli;
        struct connection *conn;

        for (int i = 0; i < ctx->cnf->max_acp; i++) {
                cli = socket_accept(fd, NULL);

                if (cli < 0)
                        break;

                set_nonblock(cli);

                if (ev_add(ctx->epfd, cli, EPOLLIN | EPOLLET, conn) < 0) {
                        close(cli);
                        free(conn);
                        continue;
                }
        }
}

static void gateway_read(struct connection *conn)
{
        conn->heartbeat = time(NULL);
}

static void gateway_event_loop(struct gateway *ctx)
{
        int n;
        struct epoll_event *evt;
        struct epoll_event events[ctx->cnf->max_evt];

        n = epoll_wait(ctx->epfd, events, (int) ctx->cnf->max_evt, -1);

        for (int i = 0; i < n; i++) {
                evt = &events[i];

                if (evt->data.fd == ctx->listen_fd) {
                        gateway_new_connection(ctx, evt->data.fd);
                        continue;
                }

                gateway_read((struct connection *) evt->data.ptr);
        }
}

__attr_noreturn
void gateway_start(struct config *cnf)
{
        struct gateway ctx;

        gateway_init(&ctx, cnf);

        ctx.listen_fd = socket_start(cnf->port);

        if (ctx.listen_fd < 0) {
                perror("socket_start");
                exit(1);
        }

        set_nonblock(ctx.listen_fd);

        ctx.epfd = epoll_create1(EPOLL_CLOEXEC);

        if (ctx.epfd < 0) {
                perror("epoll_create1 failed");
                goto err;
        }

        if (ev_add(ctx.epfd, ctx.listen_fd, EPOLLIN, NULL) != 0)
                goto err;

        while (1)
                gateway_event_loop(&ctx);

err:
        gateway_destroy(&ctx);
}

