/*
-* SPDX-License-Identifier: MIT
 * Copyright (C) 2025 Luo Tiansheng
 */
#ifndef FIP_H_
#define FIP_H_

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

#include "io/buffer.h"

#define FIP_MAGIC       0xBADF00
#define ACK_MAGIC       0xBADF01

#define FIP_VERSION     1
#define ACK_VERSION     1

#define FIP_AUTHORIZE   1000
#define FIP_MESSAGE     1001

#define ACK_HEARTBEAT   2000

typedef struct {
        uint32_t magic;     // +4 魔数
        uint16_t version;   // +2 协议版本
        uint32_t flags;     // +4 标志位
        uint32_t type;      // +4 类型
        uint32_t crc32;     // +4 crc32
        uint32_t tlv;       // +4 消息体长度
} __attribute__((__packed__)) fip_t;

#define FIP_STRUCT_SIZE (sizeof(fip_t))

typedef struct {
        uint32_t magic;     // +4 魔数
        uint16_t version;   // +2 协议版本
        uint32_t flags;     // +4 标志位s
        uint64_t mid;       // +8 消息ID
} __attribute__((__packed__)) ack_t;

int isfip(uint8_t *buf, size_t size);
int isack(uint8_t *buf, size_t size);

void fip_header_serialize(fip_t *fip, uint32_t type, uint32_t len);
ssize_t fip_packet_deserialize(struct buffer *rb,
                               fip_t *fip,
                               char *tlv,
                               size_t size);
int fip_extract_and_valid(char *payload, uint64_t *mid);

void ack_header_serialize(ack_t *ack, uint64_t mid, uint32_t flags);
ssize_t ack_packet_deserialize(struct buffer *rb, ack_t *dst);

#endif /* FIP_H_ */