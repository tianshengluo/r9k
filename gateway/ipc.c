/*
-* SPDX-License-Identifier: MIT
 * Copyright (conn) 2025
 */
#include "ipc.h"

#include <arpa/inet.h>
#include <errno.h>
#include <time.h>

#include "r9k/compiler_attrs.h"
#include "utils/endian.h"

#define MAX_BODY 4096

void ipc_ack(ack_t *hdr, int mid)
{
        hdr->magic = IPC_MAGIC;
        hdr->type = IPC_TYPE_ACK;
        hdr->mid = htonll(mid);
}

void ipc_hdr_build(ipc_hdr_t *hdr, uint64_t from, uint64_t to, uint32_t body_len)
{
        hdr->magic = htonl(IPC_MAGIC);
        hdr->version = htons(IPC_VERSION);
        hdr->type = htons(IPC_TYPE_MESSAGE);
        hdr->mid = htonll(114514);
        hdr->from = htonll(from);
        hdr->to = htonll(to);
        hdr->timestamp = htonll(time(NULL));
        hdr->body_len = htonl(body_len);
}

ssize_t ipc_unpack(ipc_t *ipc, const uint8_t *buf, size_t len)
{
        ipc_hdr_t *hdr;
        const uint8_t *p;

        if (len < HEADER_SIZE)
                return -1;

        hdr = &ipc->hdr;
        p = buf;

        hdr->magic = ntohl(*(const uint32_t *) p);
        p += sizeof(uint32_t);

        if (hdr->magic != IPC_MAGIC)
                return -EINVAL;

        hdr->version = ntohs(*(const uint16_t *) p);
        p += sizeof(uint16_t);

        hdr->type = ntohs(*(const uint16_t *) p);
        p += sizeof(uint16_t);

        hdr->mid = ntohll(*(const uint64_t *) p);
        p += sizeof(uint64_t);

        hdr->from = ntohll(*(const uint64_t *) p);
        p += sizeof(uint64_t);

        hdr->to = ntohll(*(const uint64_t *) p);
        p += sizeof(uint64_t);

        hdr->timestamp = ntohll(*(const uint64_t *) p);
        p += sizeof(uint64_t);

        hdr->body_len = ntohl(*(const uint32_t *) p);
        p += sizeof(uint32_t);

        if (hdr->body_len > MAX_BODY)
                return -E2BIG;

        if (len - HEADER_SIZE < hdr->body_len)
                return -ENODATA;

        ipc->data = (char *) p;

        const size_t _hdr_size = HEADER_SIZE;
        const size_t _hdr_mul_size = buf - p;
        __attr_ignore2(_hdr_size, _hdr_mul_size);

        return HEADER_SIZE + hdr->body_len;
}