/*
-* SPDX-License-Identifier: MIT
 * Copyright (C) 2025 Luo Tiansheng
 */
#include <r9k/io_utils.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#define BUFSIZE 4096

typedef ssize_t (*io_rw_t)(int fd, void *buf, size_t count);

static ssize_t _rw_read(int fd, void *buf, size_t count)
{
        return read(fd, buf, count);
}

static ssize_t _rw_write(int fd, void *buf, size_t count)
{
        return write(fd, buf, count);
}

static char *_read_in(FILE *stream)
{
        size_t len = 0;
        size_t size = BUFSIZE;
        char *buf = malloc(size + 1);

        if (!buf) {
                perror("malloc");
                return NULL;
        }

        size_t n;

        while ((n = fread(buf + len, 1, size, stream)) > 0) {
                len += n;

                if (len >= size) {
                        size *= 2;
                        char *tmp = realloc(buf, size + 1);

                        if (!tmp) {
                                perror("realloc");
                                free(buf);
                                return NULL;
                        }

                        buf = tmp;
                }
        }

        if (ferror(stream)) {
                perror("read stdin");
                free(buf);
                return NULL;
        }

        buf[len] = '\0';
        return buf;
}

char *slurp(FILE *stream)
{
        char *buf;
        long fsize;
        size_t nread;
        size_t cap;

        if (!stream) {
                errno = EBADF;
                return NULL;
        }

        if (stream == stdin)
                return _read_in(stream);

        if (fseek(stream, 0, SEEK_END) != 0)
                return NULL;

        fsize = ftell(stream);
        if (fsize < 0)
                return NULL;

        cap = (size_t) fsize + 1;
        rewind(stream);

        buf = malloc(cap);
        if (!buf)
                return NULL;

        nread = fread(buf, 1, fsize, stream);
        if (ferror(stream)) {
                free(buf);
                return NULL;
        }

        rewind(stream);

        buf[nread] = '\0';
        return buf;
}

char *readfile(const char *path)
{
        FILE *stream = fopen(path, "rb");
        if (!stream)
                return NULL;

        char *buf = slurp(stream);
        fclose(stream);

        return buf;
}

ssize_t writefile(const char *path, const void *buf, size_t count)
{
        FILE *stream;
        ssize_t r;

        if (!buf) {
                errno = EINVAL;
                return -1;
        }

        if (count == 0)
                return 0;

        stream = fopen(path, "wb");
        if (!stream)
                return -1;

        r = fwrite(buf, 1, count, stream);
        fclose(stream);

        return r;
}

static ssize_t _io_buffer_loop(int fd, const void *buf, size_t count, io_rw_t rw)
{
        if (fd < 0 || !buf) {
                errno = EINVAL;
                return -1;
        }

        if (!count)
                return 0;

        size_t n = 0;

        while (n < count) {
                ssize_t r;

                r = rw(fd, (uint8_t *) buf + n, count - n);

                if (r > 0) {
                        n += r;
                        continue;
                }

                if (r == -1) {
                        if (errno == EINTR)
                                continue;
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                                return n;
                        return -1;
                }
        }

        return n;
}

ssize_t io_buffer_read(int fd, void *buf, size_t count)
{
        return _io_buffer_loop(fd, buf, count, _rw_read);
}

ssize_t io_buffer_write(int fd, const void *buf, size_t count)
{
        return _io_buffer_loop(fd, buf, count, _rw_write);
}