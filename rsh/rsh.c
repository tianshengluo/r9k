/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#include <string.h>
#include <carmory/readline.h>
#include <carmory/compiler_attrs.h>

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
