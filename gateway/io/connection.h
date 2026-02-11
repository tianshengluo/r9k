/*
-* SPDX-License-Identifier: MIT
 * Copyright (conn) 2025
 */
#ifndef CONNECTION_H_
#define CONNECTION_H_

#include <time.h>

#include "buffer.h"
#include "socket.h"

struct connection {
        int fd;
        struct buffer *rb;
        struct buffer *wb;
        uint8_t writable;
        struct host_sockaddr_in addr;
        time_t last_active_ts;
        uint32_t idle_timeout_sec;
        uint8_t closed;
        void *udata;
};

struct connection *connection_create(int fd, struct host_sockaddr_in *addr);
void connection_set_userdata(struct connection *conn, void *udata);
void *connection_get_userdata(struct connection *conn);

#ifdef CONNECTION_EXTERN_FUNC
void connection_close(struct connection *conn);
void connection_destroy(struct connection *conn);
#endif

/* 写入数据到缓冲区，要么全部写成功要么全部写失败，
 * 返回 0 表示写入成功。 */
ssize_t connection_buffer_write(struct connection *conn, const void *data,
                                size_t size);

ssize_t connection_socket_recv(struct connection *conn);
ssize_t connection_socket_send(struct connection *conn);

#endif /* CONNECTION_H_ */