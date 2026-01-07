/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Varketh Nockrath
 */
#ifndef IO_UTILS_H_
#define IO_UTILS_H_

#include <stdio.h>
#include <unistd.h>

void	  *slurp(int fd, size_t *len);
ssize_t	dump(int fd, const void *buf, size_t count);
void	  *fslurp(FILE *stream, size_t *len);
ssize_t	fdump(FILE *stream, const void *buf, size_t count);

char	  *stream_read_c(FILE *stream);
void   *path_read_v(const char *path, size_t *len);
char	  *path_read(const char *path);
ssize_t path_write(const char *path, const void *buf, size_t count);

/* io_buffer_read/io_buffer_write: Read/write up to count bytes from/to fd using buf,
 * handle EINTR/EAGAIN, return bytes processed or -1 on error. */
ssize_t io_buffer_read(int fd, void *buf, size_t count);
ssize_t io_buffer_write(int fd, const void *buf, size_t count);

#endif /* IO_UTILS_H_ */
