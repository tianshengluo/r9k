/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Varketh Nockrath
 */
#ifndef BASE64_H_
#define BASE64_H_

#include <stdlib.h>

char *base64_encode(const unsigned char *data, size_t len);
unsigned char *base64_decode(const char *b64, size_t *out_len);

#endif /* BASE64_H_ */