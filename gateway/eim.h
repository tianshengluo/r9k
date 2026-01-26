/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 *
 * Enterprise Instant Messaging.
 */
#ifndef EIM_H_
#define EIM_H_

#include <stddef.h>
#include "socket.h"

/* #define EIM_MAKE_VERSION(major, minor, patch) \
        (((major) << 24) | ((minor) << 16) | (patch)) */

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
        EIM_TYPE_MAX     = 16,
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
        char    *data;          /* data */
} __attribute__((packed));

#define EIM_SIZE offsetof(struct eim, data)
#define EIM_ACK_SIZE offsetof(struct eim, sid)

static inline eim_error_t eimrbvalid(struct eim *eim, size_t rb_size)
{
        if (ntohl(eim->magic) != EIM_MAGIC)
                return EIM_ERROR_MAGIC;

        if (ntohs(eim->version) != EIM_VERSION)
                return EIM_ERROR_VERSION;

        if (ntohs(eim->type) > EIM_TYPE_MAX)
                return EIM_ERROR_TYPE;

        if (ntohl(eim->len) > rb_size - EIM_SIZE)
                return EIM_ERROR_INCOMPLETE;

        return EIM_OK;
}

static inline ssize_t eim(uint8_t *rb, size_t size, struct eim **p_eim)
{
        if (size < EIM_SIZE)
                return EIM_ERROR_INCOMPLETE;

        eim_error_t error;
        struct eim *eim = (struct eim *) rb;

        if ((error = eimrbvalid(eim, size)) != EIM_OK)
                return error;

        eim->magic = ntohl(eim->magic);
        eim->mid = ntohll(eim->mid);
        eim->sid = ntohll(eim->sid);
        eim->from = ntohll(eim->from);
        eim->to = ntohll(eim->to);
        eim->time = ntohll(eim->time);
        eim->len = ntohl(eim->len);

        eim->data = (char *) (rb + EIM_SIZE);
        eim->data[eim->len] = '\0';

        *p_eim = eim;

        return EIM_SIZE + eim->len;
}

static inline void ack(struct eim *p_eim)
{
        p_eim->magic   = htonl(EIM_MAGIC);
}

#endif /* EIM_H_ */