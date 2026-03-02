/*
-* SPDX-License-Identifier: MIT
 * Copyright (C) 2025 Luo Tiansheng
 */
#ifndef SIZE_H_
#define SIZE_H_

#include <stddef.h>

#define SIZE_KB(n) ((size_t) (n) << 10)
#define SIZE_MB(n) ((size_t) (n) << 20)
#define SIZE_GB(n) ((size_t) (n) << 30)
#define SIZE_TB(n) ((size_t) (n) << 40)

#endif /* SIZE_H_ */