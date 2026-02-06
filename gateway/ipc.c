/*
-* SPDX-License-Identifier: MIT
 * Copyright (conn) 2025
 */
#include "ipc.h"

#include <arpa/inet.h>
#include <errno.h>
#include <r9k/yyjson.h>

#define IPC_STRUCT_SIZE (sizeof(ipc_t))

ssize_t ipc_header_unpack(ipc_t *ipc, uint8_t *buf, size_t size)
{
        if (size < IPC_STRUCT_SIZE)
                return -ENODATA;

        ssize_t off = 0;

        ipc->magic = ntohl(*(uint32_t *) (buf + off));
        off += sizeof(uint32_t);

        if (ipc->magic != IPC_MAGIC)
                return -EPROTO;

        ipc->version = ntohs(*(uint16_t *) (buf + off));
        off += sizeof(uint16_t);

        ipc->flags = ntohl(*(uint32_t *) (buf + off));
        off += sizeof(uint32_t);

        ipc->type = ntohl(*(uint32_t *) (buf + off));
        off += sizeof(uint32_t);

        ipc->body_len = ntohl(*(uint32_t *) (buf + off));
        off += sizeof(uint32_t);

        if (ipc->body_len > (size - off))
                return -ENODATA;

        return off;
}