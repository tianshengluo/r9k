/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Varketh Nockrath
 */
#ifndef IO_UTILS_H_
#define IO_UTILS_H_

#include <stdio.h>
#include <unistd.h>

char   *slurp(FILE *stream);
char   *readfile(const char *path);
ssize_t writefile(const char *path, const void *buf, size_t count);

/* io_buffer_read/io_buffer_write: Read/write up to count bytes from/to fd using buf,
 * handle EINTR/EAGAIN, return bytes processed or -1 on error. */
ssize_t io_buffer_read(int fd, void *buf, size_t count);
ssize_t io_buffer_write(int fd, const void *buf, size_t count);

#endif /* IO_UTILS_H_ */
