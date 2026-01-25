#include "conn.h"

#include <stdlib.h>
#include <unistd.h>

struct connection *connection_create(int fd, size_t maxrb, size_t maxwb)
{
        struct connection *conn;

        conn = calloc(1, sizeof(struct connection));

        if (!conn)
                return NULL;

        conn->fd     = fd;
        conn->rb.cap = maxrb;
        conn->wb.cap = maxwb;

        /* alloc read buffer */
        conn->rb.buf = malloc(maxrb);

        if (!conn->rb.buf)
                goto err;

        /* alloc write buffer */
        conn->wb.buf = malloc(maxwb);

        if (!conn->wb.buf)
                goto err;

        return conn;

err:
        connection_destroy(conn);
        return NULL;
}


void connection_destroy(struct connection *conn)
{
        if (!conn)
                return;

        if (conn->rb.buf)
                free(conn->rb.buf);

        if (conn->wb.buf)
                free(conn->wb.buf);

        if (conn->fd >= 0)
                close(conn->fd);

        free(conn);
}