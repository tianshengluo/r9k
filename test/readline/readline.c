/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Luo Tiansheng
 *
 */
#include <stdio.h>
#include <carmory/readline.h>

int main(int argc, char *argv[])
{
        while (1) {
                const char *str = readline("carmory > ");

                if (!str)
                        continue;

                printf("%s\n", str);
        }
}
