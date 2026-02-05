/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#ifndef BUFFER_H_
#define BUFFER_H_

#include <stdio.h>
#include <stdint.h>
#include <r9k/compiler_attrs.h>

struct buffer {
        uint8_t *base;
        size_t   cap;
        size_t   rpos;
        size_t   wpos;
};

struct buffer *buffer_alloc(size_t size);
void buffer_free(struct buffer *buf);
void buffer_compact(struct buffer *buf);

/**
 * 获取 buffer 中可用内存大小，如果内存不足则会尝试压缩 buffer 腾出
 * 可写入空间。若还是返回 0，表示没有可继续写入空间。
 */
size_t buffer_writeable(struct buffer *buf);
size_t buffer_readable(struct buffer *buf);

__attr_always_inline
static inline size_t buffer_length(struct buffer *buf)
{
        return buf->wpos - buf->rpos;
}

#endif /* BUFFER_H_ */