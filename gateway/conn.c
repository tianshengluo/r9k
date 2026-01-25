#include "conn.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>

#include "socket.h"

struct connection *connection_create(int sock_fd, int epfd, size_t maxrb, size_t maxwb)
{
        struct connection *conn;

        conn = calloc(1, sizeof(struct connection));

        if (!conn)
                return NULL;

        conn->sock_fd = sock_fd;
        conn->epoll_fd = epfd;
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

        if (conn->sock_fd >= 0)
                close(conn->sock_fd);

        free(conn);
}

int connection_read(struct connection *conn)
{
        struct stagbuf *rb = &conn->rb;

        while (1) {
                ssize_t n = recv(conn->sock_fd,
                                 rb->buf + rb->len,
                                 rb->cap - rb->len,
                                 0);

                if (n == 0 || rb->len >= rb->cap) {
                        connection_destroy(conn);
                        return -1;
                }

                if (n < 0) {
                        RETRY_ONCE_ENTR();

                        if (is_eagain())
                                break;
                }

                rb->len += n;
        }

        return 0;
}

int connection_write(struct connection *conn, const void *data, size_t size)
{
        struct stagbuf *wb = &conn->wb;

        if (wb->cap - wb->len < size)
                return -1;

        struct epoll_event event = {
                .events = EPOLLIN | EPOLLOUT | EPOLLET,
                .data.ptr = conn,
        };

        if (epoll_ctl(conn->epoll_fd, EPOLL_CTL_MOD, conn->sock_fd, &event) < 0) {
                return -1;
        }

        memcpy(wb->buf + wb->len, data, size);
        wb->len += size;

        return 0;
}