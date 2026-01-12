/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Varketh Nockrath
 */
#include <r9k/io_utils.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <r9k/trace.h>
#include <r9k/size.h>

#define SLURP_BUFF_MAX (SIZE_GB * 4UL)

typedef ssize_t (*io_rw_t)(int fd, void *buf, size_t count);

static ssize_t _rw_read(int fd, void *buf, size_t count)
{
        return read(fd, buf, count);
}

static ssize_t _rw_write(int fd, void *buf, size_t count)
{
        return write(fd, buf, count);
}

void *slurp(int fd, size_t *len)
{
        uint8_t *buf = NULL;
        uint8_t *tmp;
        size_t cap= SIZE_KB * 4;
        size_t nread = 0;
        uint8_t is_stream = 1;
        struct stat st;

        if (fd < 0) {
                errno = EBADF;
                return NULL;
        }

        if (fstat(fd, &st) == 0 && S_ISREG(st.st_mode)) {
                if (st.st_size < 0) {
                        errno = EOVERFLOW;
                        return NULL;
                }

                if ((uintmax_t) st.st_size > SLURP_BUFF_MAX) {
                        errno = ENOMEM;
                        return NULL;
                }

                cap = (size_t) st.st_size;
                is_stream = 0;
        }

        buf = malloc(cap);
        if (!buf)
                return NULL;

        while (1) {
                ssize_t n;

                if (is_stream && cap - nread < SIZE_KB * 4) {
                        cap += cap / 2;
                        if (cap > SLURP_BUFF_MAX) {
                                errno = ENOMEM;
                                goto err;
                        }

                        tmp = realloc(buf, cap);
                        if (!tmp)
                                goto err;

                        buf = tmp;
                }

                n = read(fd, buf + nread, cap - nread);
                if (n == 0)
                        break;

                if (n > 0) {
                        nread += n;
                        continue;
                }

                if (errno == EINTR
                        || errno == EAGAIN
                        || errno == EWOULDBLOCK)
                        continue;

                goto err;
        }

        if (is_stream && cap > nread) {
                tmp = realloc(buf, nread);
                if (tmp)
                        buf = tmp;
        }

        if (len)
                *len = nread;

        return buf;

err:
        free(buf);
        return NULL;
}

ssize_t dump(int fd, const void *buf, size_t count)
{
        if (fd < 0 || !buf) {
                errno = EINVAL;
                return -1;
        }

        if (count == 0)
                return 0;

        ssize_t n;
        size_t written = 0;

        while (written < count) {
                n = write(fd, (const uint8_t *) buf + written, count - written);

                if (n > 0) {
                        written += n;
                        continue;
                }

                if (n == -1 && errno == EINTR)
                        continue;

                return -1;
        }

        return written;
}

void *fslurp(FILE *stream, size_t *len)
{
        if (!stream) {
                errno = EINVAL;
                return NULL;
        }

        return slurp(fileno(stream), len);
}

ssize_t fdump(FILE *stream, const void *buf, size_t count)
{
        if (!stream || !buf) {
                errno = EINVAL;
                return -1;
        }

        return dump(fileno(stream), buf, count);
}

char *stream_read_c(FILE *stream)
{
        if (!stream) {
                errno = EINVAL;
                return NULL;
        }

        char *buf;
        size_t size;

        buf = fslurp(stream, &size);
        if (!buf)
                return NULL;

        char *tmp = realloc(buf, size + 1);
        if (!tmp) {
                free(buf);
                return NULL;
        }

        buf = tmp;
        buf[size] = '\0';

        return buf;
}

void *path_read_v(const char *path, size_t *len)
{
        FILE *stream = fopen(path, "rb");
        if (!stream)
                return NULL;

        char *buf = fslurp(stream, len);
        fclose(stream);

        return buf;
}

char *path_read(const char *path)
{
        FILE *stream = fopen(path, "rb");
        if (!stream)
                return NULL;

        char *buf = stream_read_c(stream);
        fclose(stream);

        return buf;
}

ssize_t path_write(const char *path, const void *buf, size_t count)
{
        if (!buf) {
                errno = EINVAL;
                return -1;
        }

        if (count == 0)
                return 0;

        FILE *stream = fopen(path, "wb");
        if (!stream)
                return -1;

        ssize_t r = fdump(stream, buf, count);

        if (fclose(stream) != 0)
                return -1;

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