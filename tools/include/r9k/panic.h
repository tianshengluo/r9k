/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 */
#ifndef PANIC_H_
#define PANIC_H_

#include <stdio.h>
#include <stdlib.h>

#define PANIC(fmt, ...)                                 \
        do {                                            \
                fprintf(stderr, fmt, ##__VA_ARGS__);    \
                exit(EXIT_FAILURE);                     \
        } while(0)

#define PANIC_IF(cond, fmt, ...)                        \
        do {                                            \
                if (cond)                               \
                        PANIC(fmt, ##__VA_ARGS__);      \
        } while(0)

#endif /* PANIC_H_ */