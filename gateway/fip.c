/*
-* SPDX-License-Identifier: MIT
 * Copyright (C) 2025 Luo Tiansheng
 */
#include "fip.h"

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
static inline uint32_t _fip_magic(uint8_t *buf, size_t size)
{
        if (size < sizeof(uint32_t))
                return 0;

        return ntohl(*(uint32_t *) buf);
}

static ssize_t _fip_buffer_valid(fip_t *fip, uint8_t *buf, size_t size)
{
        if (size < FIP_STRUCT_SIZE)
                return -ENODATA;

        ssize_t off = 0;

        if (!isfip(buf, size))
                return -EPROTO;

        fip->magic = FIP_MAGIC;
        off += sizeof(uint32_t);

        fip->version = ntohs(*(uint16_t *) (buf + off));
        off += sizeof(uint16_t);

        fip->flags = ntohl(*(uint32_t *) (buf + off));
        off += sizeof(uint32_t);

        fip->type = ntohl(*(uint32_t *) (buf + off));
        off += sizeof(uint32_t);

        fip->crc32 = ntohl(*(uint32_t *) (buf + off));
        off += sizeof(uint32_t);

        fip->tlv = ntohl(*(uint32_t *) (buf + off));
        off += sizeof(uint32_t);

        if (fip->tlv > MAX_TLV)
                return -EMSGSIZE;

        if (fip->tlv > (size - off))
                return -ENODATA;

        return FIP_STRUCT_SIZE;
}

int isfip(uint8_t *buf, size_t size)
{
        return _fip_magic(buf, size) == FIP_MAGIC;
}

int isack(uint8_t *buf, size_t size)
{
        return _fip_magic(buf, size) == ACK_MAGIC;
}

void fip_header_serialize(fip_t *fip, uint32_t type, uint32_t len)
{
        fip->magic = htonl(FIP_MAGIC);
        fip->version = htons(FIP_VERSION);
        fip->flags = htonl(0);
        fip->type = htonl(type);
        fip->crc32 = htonl(0);
        fip->tlv = htonl(len);
}

ssize_t fip_packet_deserialize(struct buffer *rb,
                               fip_t *fip,
                               char *tlv,
                               size_t size)
{
        uint8_t *buf;
        ssize_t r;

        buf = buffer_peek_rcur(rb);

        r = _fip_buffer_valid(fip, buf, buffer_readable(rb));

        if (r > 0) {
                if (size < fip->tlv + 1)
                        return -ENOBUFS;

                memcpy(tlv, buf + r, fip->tlv);
                tlv[fip->tlv] = '\0';

                return buffer_skip_rpos(rb, r + fip->tlv);
        }

        switch (r) {
                case -EPROTO:
                        log_error("invalid protocol data, parse fip_t failed\n");
                        return r;
                case -ENODATA:
                        return r;
                case -EMSGSIZE:
                        return r;
                default:
                        log_error("unknown fip_unpack_buffer() return errno: %ld\n", r);
                        return r;
        }
}

int fip_extract_and_valid(char *payload, uint64_t *mid)
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

ssize_t ack_packet_deserialize(struct buffer *rb, ack_t *dst)
{
        uint8_t *buf = buffer_peek_rcur(rb);

        if (!isack(buf, buffer_readable(rb)))
                return -EINVAL;

        size_t off = 0;

        dst->magic = ntohl(*(uint32_t *) buf + off);
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