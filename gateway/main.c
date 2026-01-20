/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#include "gateway.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <r9k/compiler_attrs.h>

int main(int argc, char **argv)
{
        __attr_ignore2(argc, argv);

        struct config cnf;
        struct rlimit rl;

        cnf.port    = 26214;
        cnf.max_evt = 8192;

        if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
                perror("getrlimit() failed");
                exit(1);
        }

        cnf.limit = rl.rlim_cur;

        /* start */
        gateway_start(&cnf);

        return 0;
}

