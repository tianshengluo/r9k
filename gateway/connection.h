/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Varketh Nockrath
 */
#ifndef CONNECTION_H_
#define CONNECTION_H_

struct connection {
        uint8_t *stagbuf;
        size_t   cap;
        size_t   len;
        size_t   off;
        int      fd;
};

static struct connection *connection_wrap(int fd)
{
        struct connection *conn;

        conn = calloc(1, sizeof(struct connection));

        if (!conn)
                return NULL;

        conn->fd = fd;

        return conn;
}

static struct connection *connection_new(int fd, size_t cap)
{
        struct connection *conn;

        conn = connection_wrap(fd);

        if (!conn)
                return NULL;

        conn->cap     = cap;
        conn->len     = 0;
        conn->stagbuf = malloc(cap);

        if (!conn->stagbuf) {
                free(conn);
                return NULL;
        }

        return conn;
}

static void connection_close(struct connection *conn)
{
        if (!conn)
                return;

        if (conn->stagbuf)
                free(conn->stagbuf);

        close(conn->fd);
        free(conn);
}

#endif /* CONNECTION_H_ */