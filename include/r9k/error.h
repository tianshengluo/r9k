/*
-* SPDX-License-Identifier: MIT
 * Copyright (C) 2025 Luo Tiansheng
 */
#ifndef CINAP_H_
#define CINAP_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <r9k/compiler_attrs.h>

__attr_unused
__attr_printf(1, 2)
static void _panic(const char *fmt, ...)
{
        va_list va;
        va_start(va, fmt);
        vfprintf(stderr, fmt, va);
        va_end(va);
        exit(EXIT_FAILURE);
}

#define PANIC(fmt, ...) _panic(fmt, ##__VA_ARGS__)

#define PANIC_IF(cond, fmt, ...)                        \
        do {                                            \
                if (cond)                               \
                        _panic(fmt, ##__VA_ARGS__);     \
        } while(0)

#define FATAL() exit(1)

#endif /* CINAP_H_ */