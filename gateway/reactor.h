/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 *
 * Enterprise Instant Messaging.
 */
#ifndef REACTOR_H_
#define REACTOR_H_

#include <sys/epoll.h>
#include "socket.h"

#define RC_READ  (1 << 0)
#define RC_WRITE (1 << 1)
#define RC_ET    (1 << 2)

typedef struct connection connection_t;
typedef struct reactor reactor_t;
typedef struct stagbuf stagbuf_t;

typedef void (*fn_on_event) (connection_t *conn);

struct stagbuf {
        uint8_t                *buf;
        size_t                  cap;
        size_t                  len;
        size_t                  off;
};

struct connection {
        int                     fd;
        reactor_t              *rc;
        stagbuf_t               rb;
        stagbuf_t               wb0;
        stagbuf_t               wb1;
};

struct reactor {
        int                     epfd;
        int                     listen_fd;
        size_t                  maxevents;
        struct epoll_event     *event_array;
        int                     event_count;
        fn_on_event             on_event_read;
        fn_on_event             on_event_write;
};

reactor_t *rc_create(int listen_fd, size_t maxevents);
void rc_destroy(reactor_t *rc);
void rc_set_read_callback(reactor_t *rc, fn_on_event on_event_read);
void rc_set_write_callback(reactor_t *rc, fn_on_event on_event_write);
int  rc_event_add(reactor_t *rc, int fd, uint32_t events, void *userdata);
int  rc_event_mod(reactor_t *rc, int fd, uint32_t events, void *userdata);
int  rc_event_del(reactor_t *rc, int fd);
void rc_poll_events(reactor_t *rc);

connection_t *connection_create(reactor_t *rc, int fd, size_t maxrb, size_t maxwb0, size_t maxwb1);
void connection_destroy(connection_t *conn);

#endif /* REACTOR_H_ */