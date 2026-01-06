/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Varketh Nockrath
 */
#ifndef IO_UTILS_H_
#define IO_UTILS_H_

#include <stdio.h>

void	*slurp(int fd, size_t *len);
ssize_t	 dump(int fd, const void *buf, size_t count);
void	*fslurp(FILE *stream, size_t *len);
ssize_t	 fdump(FILE *stream, const void *buf, size_t count);
char	*readfile(FILE *stream);
ssize_t	 writefile(FILE *stream, const void *buf, size_t count);
char	*readpath(const char *path);
ssize_t	 writepath(const char *path, const void *buf, size_t count);
ssize_t	 readall(int fd, void *buf, size_t count);
ssize_t	 writeall(int fd, const void *buf, size_t count);

#endif /* IO_UTILS_H_ */
