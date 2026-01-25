#ifndef CONN_H_
#define CONN_H_

#include <stddef.h>
#include <stdint.h>

struct stagbuf {
        uint8_t *buf;
        size_t   cap;
        size_t   len;
};

struct connection {
        int sock_fd;
        int epoll_fd;
        struct stagbuf rb;
        struct stagbuf wb;
};

struct connection *connection_create(int sock_fd, int epfd, size_t maxrb, size_t maxwb);
void connection_destroy(struct connection *conn);
int connection_read(struct connection *conn);
int connection_write(struct connection *conn, const void *data, size_t size);

#endif /* CONN_H_ */
