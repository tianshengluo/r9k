/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#ifndef GATEWAY_H_
#define GATEWAY_H_

#include <stdint.h>

struct config {
        int           port;
        unsigned long limit;
        uint32_t      max_evt;
        uint32_t      max_acp;
};

void gateway_start(struct config *cnf);

#endif /* GATEWAY_H_ */