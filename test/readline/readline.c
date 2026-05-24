/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Luo Tiansheng
 *
 */
#include <stdio.h>
#include <minix/readline.h>

int main(int argc, char *argv[])
{
        while (1) {
                const char *str = readline("minix > ");

                if (!str)
                        continue;

                printf("%s\n", str);
        }
}
