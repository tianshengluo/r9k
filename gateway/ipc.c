/*
-* SPDX-License-Identifier: MIT
 * Copyright (conn) 2025
 */
#include "ipc.h"

#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <errno.h>
#include <r9k/compiler_attrs.h>
#include <r9k/yyjson.h>

#include "config.h"
#include "utils/log.h"
#include "utils/endian.h"

__attr_always_inline
static inline uint32_t _ipc_magic(uint8_t *buf, size_t size)
{
        if (size < sizeof(uint32_t))
                return 0;

        return ntohl(*(uint32_t *) buf);
}

static ssize_t _ipc_buffer_valid(ipc_t *ipc, uint8_t *buf, size_t size)
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

        ipc->tlv = ntohl(*(uint32_t *) (buf + off));
        off += sizeof(uint32_t);

        if (ipc->tlv > MAX_TLV)
                return -EMSGSIZE;

        if (ipc->tlv > (size - off))
                return -ENODATA;

        return IPC_STRUCT_SIZE;
}

int isipc(uint8_t *buf, size_t size)
{
        return _ipc_magic(buf, size) == IPC_MAGIC;
}

int isack(uint8_t *buf, size_t size)
{
        return _ipc_magic(buf, size) == ACK_MAGIC;
}

void ipc_header_serialize(ipc_t *ipc, uint32_t len)
{
        ipc->magic = htonl(IPC_MAGIC);
        ipc->version = htons(IPC_VERSION);
        ipc->flags = htonl(0);
        ipc->type = htonl(0);
        ipc->crc32 = htonl(0);
        ipc->tlv = htonl(len);
}

ssize_t ipc_proto_deserialize(struct buffer *rb, char *dst, size_t size)
{
        ipc_t ipc;
        uint8_t *buf;
        ssize_t r;

        buf = buffer_peek_rcur(rb);

        r = _ipc_buffer_valid(&ipc, buf, buffer_readable(rb));

        if (r > 0) {
                if (size < ipc.tlv + 1)
                        return -ENOBUFS;

                memcpy(dst, buf + r, ipc.tlv);
                dst[ipc.tlv] = '\0';

                return buffer_skip_rpos(rb, r + ipc.tlv);
        }

        switch (r) {
                case -EPROTO:
                        log_error("invalid protocol data, parse ipc_t failed\n");
                        return r;
                case -ENODATA:
                        return r;
                case -EMSGSIZE:
                        return r;
                default:
                        log_error("unknown ipc_unpack_buffer() return errno: %ld\n", r);
                        return r;
        }
}

int ipc_extract_and_valid(char *payload, uint64_t *mid)
{
        yyjson_doc *doc;
        yyjson_val *root;
        yyjson_val *v_mid;
        yyjson_val *v_content;

        doc = yyjson_read(payload, strlen(payload), 0);

        if (!doc)
                return -EINVAL;

        root = yyjson_doc_get_root(doc);

        /* message id */
        v_mid = yyjson_obj_get(root, "msg_id");

        if (!v_mid || !yyjson_is_uint(v_mid))
                goto err_free;

        *mid = yyjson_get_uint(v_mid);

        /* message content */
        v_content = yyjson_obj_get(root, "msg_content");

        if (!v_content || !yyjson_is_str(v_content))
                goto err_free;

        if (yyjson_get_len(v_content) > MAX_CNT)
                goto err_free;

        yyjson_doc_free(doc);
        return 0;

err_free:
        yyjson_doc_free(doc);
        return -EINVAL;
}

void ack_header_serialize(ack_t *ack, uint64_t mid, uint32_t flags)
{
        ack->magic = htonl(ACK_MAGIC);
        ack->version = htons(ACK_VERSION);
        ack->flags = htonl(flags);
        ack->mid = htonll(mid);
}

ssize_t ack_proto_deserialize(struct buffer *rb, ack_t *dst)
{
        uint8_t *buf = buffer_peek_rcur(rb);

        if (!isack(buf, buffer_readable(rb)))
                return -EINVAL;

        size_t off = 0;

        dst->magic = ntohl(* (uint32_t *) buf + off);
        off += sizeof(uint32_t);

        dst->version = ntohs(*(uint16_t *) (buf + off));
        off += sizeof(uint16_t);

        dst->flags = ntohl(*(uint32_t *) (buf + off));
        off += sizeof(uint32_t);

        dst->mid = ntohll(*(uint64_t *) (buf + off));
        off += sizeof(uint64_t);

        buffer_skip_rpos(rb, off);

        return off;
}