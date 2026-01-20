/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 *
 * Internal Communication Protocol.
 */
#ifndef ICP_H_
#define ICP_H_

#include <stdint.h>
#include <stddef.h>

#define ICP_MAKE_VERSION(major, minor, patch) \
        (((major) << 24) | ((minor) << 16) | (patch))

#define ICP_VERSION_1_0_0 (ICP_MAKE_VERSION(1, 0, 0))

#define ICP_MAGIC   (0x5CA1AB1E)
#define ICP_VERSION ICP_VERSION_1_0_0

struct icp {
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
        ICP_ERROR_BAD_MAGIC = -1,
        ICP_ERROR_BAD_VERSION = -2,
        ICP_ERROR_BAD_LENGTH = -3,
        ICP_ERROR_INCOMPLETE = -4,
} icp_error_t;

#define ICP_SIZE    (sizeof(struct icp))
#define MAX_STAGING (   ICP_SIZE + 8192)

icp_error_t icp_packet(char *buf, size_t *len);
void icp_build(struct icp *icp, const char *msg);

#endif /* ICP_H_ */