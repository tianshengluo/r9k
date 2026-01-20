/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 *
 * Internal Communication Protocol.
 */
#ifndef ICP_H_
#define ICP_H_

#include <stdint.h>

#include "r9k/trace.h"

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
        ICP_ERROR_BAD_VERSION,
        ICP_ERROR_BAD_LENGTH,
        ICP_ERROR_INCOMPLETE,
} icp_error_t;

#define ICP_SIZE    (sizeof(struct icp))
#define MAX_STAGING (   ICP_SIZE + 8192)

static inline icp_error_t icp_valid(struct icp *icp, size_t len)
{
        if (icp->magic != ICP_MAGIC)
                return ICP_ERROR_BAD_MAGIC;

        if (icp->version != ICP_VERSION)
                return ICP_ERROR_BAD_VERSION;

        if (icp->paksiz > len || icp->paksiz > MAX_STAGING)
                return ICP_ERROR_BAD_LENGTH;

        if (icp->paksiz < (len - ICP_SIZE))
                return ICP_ERROR_INCOMPLETE;

        return 0;
}

/* return 1 mean data invalid, 0 success */
static inline icp_error_t icp_packet(char *buf, size_t *len)
{
        char       *pak = NULL;
        size_t      off = 0;
        icp_error_t ret = 0;
        struct icp *icp = (struct icp *) buf;

        if ((ret = icp_valid(icp, *len)) != 0)
                return ret;

        off = ICP_SIZE + icp->paksiz;
        pak = (char *) buf + off;

        pak[off] = '\0';

        printf("pak: %s\n", pak);

        memmove(buf, buf + off, *len - off);
        *len -= off;

        return ret;
}

#endif /* ICP_H_ */