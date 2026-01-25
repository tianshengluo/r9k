/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 *
 * Enterprise Instant Messaging.
 */
#ifndef EIM_H_
#define EIM_H_

#include <stdint.h>

// #define EIM_MAKE_VERSION(major, minor, patch) \
//         (((major) << 24) | ((minor) << 16) | (patch))
//
// #define EIM_VERSION_1_0 (EIM_MAKE_VERSION(1, 0, 0))

#define EIM_MAGIC   (0x5CA1AB1E)
// #define EIM_VERSION EIM_VERSION_1_0
#define EIM_VERSION 1

struct eim {
        uint32_t magic;         /* magic number */
        uint8_t  version;       /* proto version */
        uint8_t  type;          /* message type */
        uint64_t mid;           /* message id */
        uint64_t sid;           /* message session id */
        uint64_t from;          /* message sender */
        uint64_t to;            /* message receiver */
        uint64_t time;          /* message send time */
        uint32_t len;           /* body len */
} __attribute__((packed));

#define EIM_SIZE sizeof(struct eim)

#endif /* EIM_H_ */