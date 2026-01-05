/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Varketh Nockrath
 */
#ifndef IO_UTILS_H_
#define IO_UTILS_H_

#include <stdio.h>

void *slurp(FILE *stream, size_t *p_size);
int   dump(FILE *stream, const void *data, size_t size);
void *readfile(const char *filename, size_t *p_size);
int   writefile(const char *filename, const void *data, size_t size);

#endif /* IO_UTILS_H_ */
