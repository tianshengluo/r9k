/*
-* SPDX-License-Identifier: MIT
 * Copyright (C) 2025 Luo Tiansheng
 */
#include "buffer.h"

#include <stdlib.h>
#include <string.h>

struct buffer *buffer_alloc(size_t size)
{
        struct buffer *buffer = NULL;

        buffer = calloc(1, sizeof(struct buffer));

        if (!buffer)
                return NULL;

        buffer->base = malloc(size);
        buffer->cap  = size;

        if (!buffer->base) {
                free(buffer);
                return NULL;
        }
        return buffer;
}

void buffer_free(struct buffer *buf)
{
        if (!buf)
                return;

        if (buf->base)
                free(buf->base);

        free(buf);
}

void buffer_compact(struct buffer *buf)
{
        if (buf->rpos == 0)
                return;

        if (buf->rpos == buf->wpos) {
                buf->rpos = 0;
                buf->wpos = 0;
                return;
        }

        memmove(buf->base,
                buf->base + buf->rpos,
                buf->wpos - buf->rpos);

        buf->wpos -= buf->rpos;
        buf->rpos  = 0;
}

size_t buffer_writeable(struct buffer *buf)
{
        size_t avail = buf->cap - buf->wpos;

        if (avail > 0)
                return avail;

        buffer_compact(buf);

        return buf->cap - buf->wpos;
}