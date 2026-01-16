/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Varketh Nockrath
 *
 */
#include <stdio.h>
#include <string.h>
#include <r9k/ssl/md5.h>
#include <r9k/compiler_attrs.h>

int main(int argc, char *argv[])
{
        __attr_ignore2(argc, argv);

        uint8_t digest[16];
        const char *str = "hello";

        MD5_CTX md5;
        md5_init(&md5);
        md5_update(&md5, str, strlen(str));
        md5_final(&md5, digest);

        for (size_t i = 0; i < 16; i++)
                printf("%02x", digest[i]);
        putchar('\n');

        return 0;
}
