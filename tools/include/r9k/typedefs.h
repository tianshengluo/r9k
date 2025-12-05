/*
* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 */
#ifndef TYPEDEFS_H_
#define TYPEDEFS_H_

#include <stdbool.h>

#define die(fmt, ...)                           \
do {                                            \
        fprintf(stderr, fmt, ##__VA_ARGS__);    \
        exit(EXIT_FAILURE);                     \
} while (0)

#define streq(a, b) (strcmp(a, b) == 0)
#define strne(a, b) (!streq(a, b))

#endif /* TYPEDEFS_H_ */