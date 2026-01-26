/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 *
 * Internal Communication Protocol.
 */
#include "eim.h"

#include <stdint.h>
#include <time.h>
#include <string.h>

static eim_error_t eimrbvalid(eim_t *eim, size_t rb_size)
{
        if (ntohl(eim->magic) != EIM_MAGIC)
                return EIM_ERROR_MAGIC;

        if (eim->version != EIM_VERSION)
                return EIM_ERROR_VERSION;

        if (eim->type > EIM_TYPE_MAX)
                return EIM_ERROR_TYPE;

        if (ntohl(eim->len) > rb_size - EIM_SIZE)
                return EIM_ERROR_INCOMPLETE;

        return EIM_OK;
}

ssize_t eim(uint8_t *rb, size_t size, eim_t **p_eim)
{
        if (size < EIM_SIZE)
                return EIM_ERROR_INCOMPLETE;

        eim_error_t error;
        eim_t *eim = (eim_t *) rb;

        if ((error = eimrbvalid(eim, size)) != EIM_OK)
                return error;

        eim->magic = ntohl(eim->magic);
        eim->mid = be64toh(eim->mid);
        eim->sid = be64toh(eim->sid);
        eim->from = be64toh(eim->from);
        eim->to = be64toh(eim->to);
        eim->time = be64toh(eim->time);
        eim->len = ntohl(eim->len);

        eim->data = (char *) &rb[EIM_SIZE];

        *p_eim = eim;

        return EIM_SIZE + eim->len;
}

void eimb(const char *message, eim_t *eim)
{
        eim->magic = htonl(EIM_MAGIC);
        eim->version = EIM_VERSION;
        eim->type = EIM_TYPE_MESSAGE;
        eim->mid = htobe64(UINT64_MAX);
        eim->sid = htobe64(UINT64_MAX);
        eim->from = htobe64(UINT64_MAX);
        eim->to = htobe64(UINT64_MAX);
        eim->time = htobe64(time(NULL));
        eim->len = htonl(strlen(message));
}

void ack(eim_t *p_eim)
{
        p_eim->magic   = htonl(EIM_MAGIC);
}