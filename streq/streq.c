/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2025 Luo Tiansheng
 *
 */
#include <stdio.h>
#include <string.h>
#include <r9k/error.h>

int main(int argc, char *argv[])
{
        PANIC_IF(argc < 3, "error: too few arguments\n");

        int r = strcmp(argv[1], argv[2]) == 0 ? 0 : 1;
        printf("%d\n", r);

        return r;
}
