/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 *
 * Internal Communication Protocol.
 */
#include "icp.h"
#include <stdio.h>
#include <string.h>

static icp_error_t icp_valid(struct icp *icp, size_t len)
{
        if (icp->magic != ICP_MAGIC)
                return ICP_ERROR_BAD_MAGIC;

        if (icp->version != ICP_VERSION)
                return ICP_ERROR_BAD_VERSION;

        if (icp->paksiz > MAX_STAGING)
                return ICP_ERROR_BAD_LENGTH;

        if (icp->paksiz > (len - ICP_SIZE))
                return ICP_ERROR_INCOMPLETE;

        return 0;
}

/* return 1 mean data invalid, 0 success */
icp_error_t icp_packet(char *buf, size_t *len)
{
        char       *pak = NULL;
        size_t      off = 0;
        icp_error_t ret = 0;
        struct icp *icp = (struct icp *) buf;

        if ((ret = icp_valid(icp, *len)) != 0)
                return ret;

        off = ICP_SIZE + icp->paksiz;
        pak = (char *) buf + ICP_SIZE;
        pak[icp->paksiz] = '\0';

        printf("pak: %s\n", pak);

        memmove(buf, buf + off, *len - off);
        *len -= off;

        return ret;
}

void icp_build(struct icp *icp, const char *msg)
{
        memset(icp, 0, sizeof(struct icp));

        icp->magic   = ICP_MAGIC;
        icp->version = ICP_VERSION;
        icp->paksiz  = strlen(msg);
}