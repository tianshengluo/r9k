/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 *
 * Internal Communication Protocol.
 */
#ifndef EIM_H_
#define EIM_H_

#include <stdint.h>

#define EIM_MAKE_VERSION(major, minor, patch) \
        (((major) << 24) | ((minor) << 16) | (patch))

#define EIM_VERSION_1_0 (EIM_MAKE_VERSION(1, 0, 0))

#define EIM_MAGIC (0x5CA1AB1E)
#define EIM_VERSION EIM_VERSION_1_0

struct eim {
        uint32_t magic;
        uint32_t version;
        uint32_t flags;
        uint16_t msg_type;
        uint64_t session_id;
        uint64_t sender_id;
        uint64_t receiver_id;
        uint64_t paksiz;
        uint32_t crc32;
} __attribute__((packed));

typedef enum {
        EIM_ERROR_BAD_MAGIC = -1,
        EIM_ERROR_BAD_VERSION = -2,
        EIM_ERROR_BAD_LENGTH = -3,
        EIM_ERROR_INCOMPLETE = -4,
} eim_error_t;

#define EIM_SIZE    (sizeof(struct eim))
#define MAX_STAGING (EIM_SIZE + 8192)

#endif /* EIM_H_ */