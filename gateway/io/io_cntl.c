/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#include "io/io_cntl.h"

#include <sys/fcntl.h>
#include <errno.h>

int isbadf(int fd)
{
        if (fcntl(fd, F_GETFD, 0) < 0)
                if (errno == EBADF)
                        return 1;
        return 0;
}

int set_nonblock(int fd)
{
        int flags = fcntl(fd, F_GETFL, 0);

        if (flags < 0)
                return -1;

        return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}