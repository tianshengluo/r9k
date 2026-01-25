#ifndef CONN_H_
#define CONN_H_

#include <stddef.h>

struct stagbuf {
        uint8_t *buf;
        size_t   cap;
        size_t   len;
};

struct connection {
        int fd;
        struct stagbuf rb;
        struct stagbuf wb;
};

struct connection *connection_create(int fd, size_t maxrb, size_t maxwb);
void connection_destroy(struct connection *conn);

#endif /* CONN_H_ */
