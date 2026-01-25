/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 *
 * Enterprise Instant Messaging.
 */
#ifndef EIM_H_
#define EIM_H_

#include <stdint.h>
#include <arpa/inet.h>

// #define EIM_MAKE_VERSION(major, minor, patch) \
//         (((major) << 24) | ((minor) << 16) | (patch))
//
// #define EIM_VERSION_1_0 (EIM_MAKE_VERSION(1, 0, 0))

#define EIM_MAGIC   (0x5CA1AB1E)
// #define EIM_VERSION EIM_VERSION_1_0
#define EIM_VERSION  1

typedef enum {
        EIM_OK               =  0,
        EIM_ERROR_MAGIC      = -1,
        EIM_ERROR_VERSION    = -2,
        EIM_ERROR_TYPE       = -3,
        EIM_ERROR_INCOMPLETE = -4,
} eim_error_t;

typedef enum {
        EIM_TYPE_MESSAGE = 0,
        EIM_TYPE_MSG_ACK = 1,
} eim_type_t;

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
#define EIM_ACK_SIZE offsetof(struct eim, mid)

static inline eim_error_t eim(uint8_t *rb, size_t size, size_t *out_size, struct eim *p_eim)
{
        struct eim *eim = (struct eim *) rb;

        eim->magic = ntohl(eim->magic);

        if (eim->magic != EIM_MAGIC)
                return EIM_ERROR_MAGIC;

        *out_size = EIM_SIZE;

        return EIM_OK;
}

static inline void ack(struct eim *p_eim)
{
        p_eim->magic   = htonl(EIM_MAGIC);
}

#endif /* EIM_H_ */