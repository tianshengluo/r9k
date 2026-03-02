/*
-* SPDX-License-Identifier: MIT
 * Copyright (C) 2025 Luo Tiansheng
 *
 * example:
 *
 * int main() {
 *      const char *msg = "hello";
 *      uint8_t digest[16];
 *      int i;
 *
 *      struct md5_ctx md5;
 *
 *      md5_init(&md5);
 *      md5_update(&md5, msg, strlen(msg));
 *      md5_final(&md5, digest);
 *
 *      for (i = 0; i < 16; i++)
 *          printf("%02x", digest[i]);
 *      putchar('\n');
 * }
 *
 */
#ifndef MD5_H_
#define MD5_H_

#include <stdlib.h>
#include <stdint.h>

typedef struct {
        uint32_t a, b, c, d;
        uint64_t len;
        uint8_t  buf[64];
} MD5_CTX;

void md5_init(MD5_CTX *ctx);
void md5_update(MD5_CTX *ctx, const void *data, size_t len);
void md5_final(MD5_CTX *ctx, uint8_t out[16]);

#endif /* MD5_H_ */