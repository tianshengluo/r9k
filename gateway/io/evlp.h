/*
-* SPDX-License-Identifier: MIT
 * Copyright (conn) 2025
 */
#ifndef EVLP_H_
#define EVLP_H_

#include "connection.h"

typedef struct evlp evlp_t;

typedef void (*on_read_fn_t) (evlp_t *evlp, struct connection *conn);
typedef void (*on_write_fn_t) (evlp_t *evlp, struct connection *conn);
typedef void (*on_accept_fn_t) (evlp_t *evlp, struct connection *conn);

struct evlp_create_info {
        on_read_fn_t on_read;
        on_write_fn_t on_write;
        on_accept_fn_t on_accept;
};

evlp_t *evlp_create(int listen_fd, struct evlp_create_info *info);
void evlp_destroy(evlp_t *evlp);
void evlp_poll_events(evlp_t *evlp);

void evlp_enable_write(evlp_t *evlp, struct connection *conn);
void evlp_disable_write(evlp_t *evlp, struct connection *conn);
void evlp_close(evlp_t *evlp, struct connection *conn);

#endif /* EVLP_H_ */