/*
-* SPDX-License-Identifier: MIT
 * Copyright (C) 2025 Luo Tiansheng
 */
#include <r9k/ssl/md5.h>
#include <stdint.h>
#include <string.h>

static const uint32_t md5_k[64] = {
	0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
	0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
	0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
	0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
	0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
	0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
	0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
	0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
	0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
	0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
	0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
	0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
	0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
	0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
	0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};

static const uint8_t md5_s[64] = {
	7,12,17,22, 7,12,17,22, 7,12,17,22, 7,12,17,22,
	5,9,14,20, 5,9,14,20, 5,9,14,20, 5,9,14,20,
	4,11,16,23, 4,11,16,23, 4,11,16,23, 4,11,16,23,
	6,10,15,21, 6,10,15,21, 6,10,15,21, 6,10,15,21
};

static inline uint32_t rol(uint32_t v, uint8_t n)
{
	return (v << n) | (v >> (32 - n));
}

static void md5_transform(MD5_CTX *ctx, const uint8_t data[64])
{
	uint32_t a = ctx->a, b = ctx->b, c = ctx->c, d = ctx->d;
	uint32_t f, g, i, m[16];

	for (i = 0; i < 16; i++)
		m[i] = (uint32_t)data[i * 4] |
		       ((uint32_t)data[i * 4 + 1] << 8) |
		       ((uint32_t)data[i * 4 + 2] << 16) |
		       ((uint32_t)data[i * 4 + 3] << 24);

	for (i = 0; i < 64; i++) {
		if (i < 16) {
			f = (b & c) | (~b & d);
			g = i;
		} else if (i < 32) {
			f = (d & b) | (~d & c);
			g = (5 * i + 1) & 15;
		} else if (i < 48) {
			f = b ^ c ^ d;
			g = (3 * i + 5) & 15;
		} else {
			f = c ^ (b | ~d);
			g = (7 * i) & 15;
		}

		f += a + md5_k[i] + m[g];
		a = d;
		d = c;
		c = b;
		b += rol(f, md5_s[i]);
	}

	ctx->a += a;
	ctx->b += b;
	ctx->c += c;
	ctx->d += d;
}

void md5_init(MD5_CTX *ctx)
{
	ctx->len = 0;
	ctx->a = 0x67452301;
	ctx->b = 0xefcdab89;
	ctx->c = 0x98badcfe;
	ctx->d = 0x10325476;
}

void md5_update(MD5_CTX *ctx, const void *data, size_t len)
{
	size_t i, idx, part;
	const uint8_t *p = data;

	idx = ctx->len & 0x3f;
	ctx->len += len;
	part = 64 - idx;

	if (len >= part) {
		memcpy(ctx->buf + idx, p, part);
		md5_transform(ctx, ctx->buf);
		for (i = part; i + 63 < len; i += 64)
			md5_transform(ctx, p + i);
		idx = 0;
		p += i;
		len -= i;
	}

	memcpy(ctx->buf + idx, p, len);
}

void md5_final(MD5_CTX *ctx, uint8_t out[16])
{
	static const uint8_t pad[64] = { 0x80 };
	uint8_t len[8];
	uint32_t i;

	for (i = 0; i < 8; i++)
		len[i] = (ctx->len << 3 >> (8 * i)) & 0xff;

	md5_update(ctx, pad, ((56 - (ctx->len & 63)) & 63));
	md5_update(ctx, len, 8);

	out[0]  = ctx->a;
	out[1]  = ctx->a >> 8;
	out[2]  = ctx->a >> 16;
	out[3]  = ctx->a >> 24;
	out[4]  = ctx->b;
	out[5]  = ctx->b >> 8;
	out[6]  = ctx->b >> 16;
	out[7]  = ctx->b >> 24;
	out[8]  = ctx->c;
	out[9]  = ctx->c >> 8;
	out[10] = ctx->c >> 16;
	out[11] = ctx->c >> 24;
	out[12] = ctx->d;
	out[13] = ctx->d >> 8;
	out[14] = ctx->d >> 16;
	out[15] = ctx->d >> 24;
}