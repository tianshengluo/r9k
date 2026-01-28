/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#ifndef TRANSPORT_H_
#define TRANSPORT_H_

typedef struct transport transport_t;

transport_t *transport_create(int listen_fd);

#endif /* TRANSPORT_H_ */