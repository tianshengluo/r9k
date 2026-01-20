/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Varketh Nockrath
 */
#include "base64.h"

#include <stdio.h>
#include <string.h>

static const char b64_map[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static signed char b64_rev[256];
static int b64_initialized = 0;

static void base64_init(void)
{
	memset(b64_rev, -1, sizeof(b64_rev));

	for (int i = 0; i < 26; i++) {
		b64_rev['A' + i] = i;
		b64_rev['a' + i] = i + 26;
	}

	for (int i = 0; i < 10; i++)
		b64_rev['0' + i] = i + 52;

	b64_rev['+'] = 62;
	b64_rev['/'] = 63;

	b64_initialized = 1;
}

char *base64_encode(const unsigned char *data, size_t len)
{
	size_t out_len = (len + 2) / 3 * 4;
	char *out = malloc(out_len + 1);
	size_t i, j;
	unsigned int v;

	if (!out) {
		perror("malloc failed");
		return NULL;
	}

	for (i = 0, j = 0; i < len; i += 3) {
		v = data[i] << 16;
		if (i + 1 < len)
			v |= data[i + 1] << 8;
		if (i + 2 < len)
			v |= data[i + 2];

		out[j++] = b64_map[(v >> 18) & 0x3f];
		out[j++] = b64_map[(v >> 12) & 0x3f];
		out[j++] = (i + 1 < len) ? b64_map[(v >> 6) & 0x3f] : '=';
		out[j++] = (i + 2 < len) ? b64_map[v & 0x3f] : '=';
	}

	out[j] = '\0';
	return out;
}

unsigned char *base64_decode(const char *b64, size_t *out_len)
{
	if (!b64_initialized)
		base64_init();

	size_t len = strlen(b64);
	size_t alloc = len / 4 * 3;
	unsigned char *out;
	size_t i, j;
	unsigned int v;
	int pad;

	if (len % 4)
		return NULL;

	if (len >= 1 && b64[len - 1] == '=') alloc--;
	if (len >= 2 && b64[len - 2] == '=') alloc--;

	out = malloc(alloc);
	if (!out) {
		perror("malloc failed");
		return NULL;
	}

	for (i = 0, j = 0; i < len; i += 4) {
		pad = 0;

		if (b64_rev[(unsigned char)b64[i]] < 0 ||
		    b64_rev[(unsigned char)b64[i + 1]] < 0)
			goto err;

		v  = b64_rev[(unsigned char)b64[i]] << 18;
		v |= b64_rev[(unsigned char)b64[i + 1]] << 12;

		if (b64[i + 2] == '=') {
			pad++;
		} else {
			if (b64_rev[(unsigned char)b64[i + 2]] < 0)
				goto err;
			v |= b64_rev[(unsigned char)b64[i + 2]] << 6;
		}

		if (b64[i + 3] == '=') {
			pad++;
		} else {
			if (b64_rev[(unsigned char)b64[i + 3]] < 0)
				goto err;
			v |= b64_rev[(unsigned char)b64[i + 3]];
		}

		out[j++] = (v >> 16) & 0xff;
		if (pad < 2)
			out[j++] = (v >> 8) & 0xff;
		if (pad < 1)
			out[j++] = v & 0xff;
	}

	if (out_len)
		*out_len = j;

	return out;

err:
	free(out);
	return NULL;
}