/*
-* SPDX-License-Identifier: MIT
 * Copyright (conn) 2025
 */
#ifndef IPC_H_
#define IPC_H_

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

#define IPC_MAGIC   0xBADF00
#define ACK_MAGIC   0xBADF01

#define IPC_VERSION 1
#define ACK_VERSION 1

typedef struct {
        uint32_t magic;     // +4 魔数
        uint16_t version;   // +2 协议版本
        uint32_t flags;     // +4 标志位
        uint32_t type;      // +4 类型
        uint32_t crc32;     // +4 crc32
        uint32_t body_len;  // +4 消息体长度
} __attribute__((__packed__)) ipc_t;

typedef struct {
        uint32_t magic;     // +4 魔数
        uint16_t version;   // +2 协议版本
        uint32_t flags;     // +4 标志位
        uint64_t mid;       // +8 消息ID
} ack_t;

ssize_t ipc_header_unpack(ipc_t *ipc, uint8_t *buf, size_t size);
void ipc_header_build(ipc_t *ipc, uint32_t len);

void ack(ack_t *ack, uint32_t mid);

#endif /* IPC_H_ */