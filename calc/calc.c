/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2025 Luo Tiansheng
 */
#include <stdio.h>

int main(int argc, char* argv[])
{
        FILE *fp;

        if (argc != 2)
                return 1;

        fp = popen("bc -l | sed 's/^\\./0./'", "w");
        if (!fp)
                return 1;

        fprintf(fp, "scale=2; %s\n", argv[1]);
        pclose(fp);

        return 0;
}