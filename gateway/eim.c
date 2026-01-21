/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 *
 * Internal Communication Protocol.
 */
#include "eim.h"
#include <stddef.h>
#include <r9k/compiler_attrs.h>

__attr_unused
static eim_error_t eim_valid(struct eim *e, size_t len)
{
        if (e->magic != EIM_MAGIC)
                return EIM_ERROR_BAD_MAGIC;

        if (e->version != EIM_VERSION)
                return EIM_ERROR_BAD_VERSION;

        if (e->paksiz > MAX_STAGING)
                return EIM_ERROR_BAD_LENGTH;

        if (e->paksiz > (len - EIM_SIZE))
                return EIM_ERROR_INCOMPLETE;

        return 0;
}