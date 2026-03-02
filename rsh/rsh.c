/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2025 Luo Tiansheng
 */
#include <string.h>
#include <r9k/readline.h>
#include <r9k/compiler_attrs.h>

int main(int argc, char* argv[])
{
        __attr_ignore2(argc, argv);

        clear();

        while (1) {
                char *line = readline("rsh > ");

                if (!line)
                        continue;

                if (strcmp(line, "clear") == 0) {
                        clear();
                        continue;
                }

                if (strcmp(line, "exit") == 0)
                        break;

                add_history(line);
        }

        return 0;
}
