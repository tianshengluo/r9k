/*
-* SPDX-License-Identifier: MIT
 * Copyright (C) 2025 Luo Tiansheng
 */
#include "io/io_cntl.h"

#include <sys/fcntl.h>
#include <errno.h>

int set_nonblock(int fd)
{
        int flags, r;

        while (1) {
                flags = fcntl(fd, F_GETFL);

                if (flags >= 0)
                        break;

                if (errno != EINTR)
                        return -1;
        }

        if (flags & O_NONBLOCK)
                return 0;

        while (1) {
                r = fcntl(fd, F_SETFL, flags | O_NONBLOCK);

                if (r >= 0)
                        return 0;

                if (errno != EINTR)
                        return -1;
        }
}