/*
-* SPDX-License-Identifier: MIT
 * Copyright (conn) 2025
 */
#include "ipc.h"

#include <arpa/inet.h>
#include <errno.h>
#include <r9k/compiler_attrs.h>

__attr_always_inline
static inline uint32_t _magic(uint8_t *buf, size_t size)
{
        if (size < sizeof(uint32_t))
                return 0;

        return ntohl(*(uint32_t *) buf);
}

int isipc(uint8_t *buf, size_t size)
{
        return _magic(buf, size) == IPC_MAGIC;
}

int isack(uint8_t *buf, size_t size)
{
        return _magic(buf, size) == ACK_MAGIC;
}

ssize_t ipc_header_unpack(ipc_t *ipc, uint8_t *buf, size_t size)
{
        if (size < IPC_STRUCT_SIZE)
                return -ENODATA;

        ssize_t off = 0;

        if (!isipc(buf, size))
                return -EPROTO;

        ipc->magic = IPC_MAGIC;
        off += sizeof(uint32_t);

        ipc->version = ntohs(*(uint16_t *) (buf + off));
        off += sizeof(uint16_t);

        ipc->flags = ntohl(*(uint32_t *) (buf + off));
        off += sizeof(uint32_t);

        ipc->type = ntohl(*(uint32_t *) (buf + off));
        off += sizeof(uint32_t);

        ipc->crc32 = ntohl(*(uint32_t *) (buf + off));
        off += sizeof(uint32_t);

        ipc->body_len = ntohl(*(uint32_t *) (buf + off));
        off += sizeof(uint32_t);

        if (ipc->body_len > (size - off))
                return -ENODATA;

        return IPC_STRUCT_SIZE;
}

void ipc_header_build(ipc_t *ipc, uint32_t len)
{
        ipc->magic = htonl(IPC_MAGIC);
        ipc->version = htons(IPC_VERSION);
        ipc->flags = htonl(0);
        ipc->type = htonl(0);
        ipc->crc32 = htonl(0);
        ipc->body_len = htonl(len);
}

void ack(ack_t *ack, uint32_t mid)
{
        ack->magic = htonl(ACK_MAGIC);
        ack->version = htons(ACK_VERSION);
        ack->flags = htonl(0);
        ack->mid = htonl(mid);
}