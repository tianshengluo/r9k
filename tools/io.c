/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Varketh Nockrath
 */
#include <r9k/io.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

void *slurp(FILE *stream, size_t *p_size)
{
        uint8_t *buf = NULL;
        uint8_t *tmp;
        size_t len = 0;
        size_t cap = 0;
        uint8_t chunk[4096];
        size_t n;

        while ((n = fread(chunk, 1, sizeof(chunk), stream)) > 0) {
                if (len + n > cap) {
                        cap = cap ? cap * 2 : sizeof(chunk);
                        if (cap < len + n)
                                cap = len + n;

                        tmp = realloc(buf, cap + 1);
                        if (!tmp) {
                                if (buf)
                                        free(buf);
                                errno = ENOMEM;
                                return NULL;
                        }
                        buf = tmp;
                }
                memcpy(buf + len, chunk, n);
                len += n;
        }

        if (ferror(stream)) {
                free(buf);
                return NULL;
        }

        if (!buf) {
                buf = malloc(1);
                if (!buf) {
                        errno = ENOMEM;
                        return NULL;
                }
        }

        buf[len] = '\0';

        if (p_size)
                *p_size = len;

        return buf;
}

int dump(FILE *stream, const void *data, size_t size)
{
        if (size == 0)
                return 0;

        if (!stream || !data) {
                errno = EINVAL;
                return -1;
        }

        const unsigned char *ptr = data;

        size_t written = 0;
        while (written < size) {
                size_t n = fwrite(ptr + written, 1, size - written, stream);
                if (n == 0) {
                        if (ferror(stream))
                                errno = EIO;
                        return -1;
                }
                written += n;
        }

        return 0;
}

void *readfile(const char *filename, size_t *p_size)
{
        void *buf;
        FILE *stream;

        stream = fopen(filename, "rb");
        if (!stream)
                return NULL;

        buf = slurp(stream, p_size);
        fclose(stream);

        return buf;
}

int writefile(const char *filename, const void *data, size_t size)
{
        if (!filename || !data) {
                errno = EINVAL;
                return -1;
        }

        FILE *stream = fopen(filename, "wb");
        if (!stream)
                return -1;

        int r = dump(stream, data, size);
        fclose(stream);

        return r;
}