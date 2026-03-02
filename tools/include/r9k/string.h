/*
-* SPDX-License-Identifier: MIT
 * Copyright (C) 2025 Luo Tiansheng
 */
#ifndef STRING_H_
#define STRING_H_

#include <string.h>

#define streq(a, b) (strcmp(a, b) == 0)

int is_str_blank(const char *str);
char *trim_start(char *str);
char *trim_end(char *str);
char *trim(char *str);

#endif /* STRING_H_ */