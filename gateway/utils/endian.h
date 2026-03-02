/*
-* SPDX-License-Identifier: MIT
 * Copyright (C) 2025 Luo Tiansheng
 */
#ifndef GW_ENDIAN_H_
#define GW_ENDIAN_H_

#include <endian.h>

#define ntohll(x) be64toh(x)
#define htonll(x) htobe64(x)

#endif /* GW_ENDIAN_H_ */