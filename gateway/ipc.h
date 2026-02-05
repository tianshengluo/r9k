/*
-* SPDX-License-Identifier: MIT
 * Copyright (conn) 2025
 */
#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <stdio.h>
#include <stdint.h>

#define IPC_MAGIC        0xBADF00
#define IPC_VERSION      1U

#define IPC_TYPE_ACK     1
#define IPC_TYPE_MESSAGE 2

typedef struct {
        uint32_t magic;
        uint16_t version;
        uint16_t type;
        uint64_t mid;
        uint64_t from;
        uint64_t to;
        uint64_t timestamp;
        uint32_t body_len;
} ipc_hdr_t;

typedef struct {
        ipc_hdr_t hdr;
        char     *data;
} ipc_t;

typedef struct {
        uint32_t magic;
        uint16_t type;
        uint64_t mid;
} ack_t;

#define HEADER_SIZE (sizeof(ipc_hdr_t))

void ipc_ack(ack_t *hdr, int mid);
void ipc_hdr_build(ipc_hdr_t *hdr, uint64_t from, uint64_t to, uint32_t body_len);
ssize_t ipc_unpack(ipc_t *ipc, const uint8_t *buf, size_t len);

#endif /* MESSAGE_H_ */