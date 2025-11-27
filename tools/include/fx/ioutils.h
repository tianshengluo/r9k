/*
* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 */
#ifndef IO_UTILS_H_
#define IO_UTILS_H_

#include <stdio.h>
#include <stdlib.h>

inline static char *freadbuf(const char *path)
{
        FILE *fp;
        size_t size;
        char *buf = NULL;
        char error[4096];

        fp = fopen(path, "r");
        if (!fp) {
                snprintf(error, sizeof(error), "open %s failed", path);
                perror(error);
                goto err;
        }

        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        buf = malloc(size);
        if (!buf) {
                snprintf(error, sizeof(error), "malloc %zu memory failed", size);
                perror("malloc failed");
                goto err;
        }

        fread(buf, 1, size, fp);
        return buf;

err:
        if (fp)
                fclose(fp);

        free(buf);

        return NULL;
}

#endif /* IO_UTILS_H_ */