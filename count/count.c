/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
//r9k
#include <r9k/argparser.h>
#include <r9k/typedefs.h>
#include <r9k/ioutils.h>

#include "wordc.h"

#define COUNT_VERSION "1.0"

static const char *strread(struct argparser *ap, struct option *f, bool *p_need_free)
{
        const char *buf = NULL;

        if (f) {
                buf = readfile(f->sval);
                if (!buf)
                        return NULL;
                *p_need_free = true;
                return buf;
        }

        if (argparser_count(ap) > 0) {
                buf = argparser_val(ap, 0);
                *p_need_free = false;
        } else {
                buf = readin();
                if (!buf)
                        return NULL;
                *p_need_free = true;
        }

        return buf;
}

int main(int argc, char **argv)
{
        struct argparser *ap;
        struct option *help, *version, *c, *l, *f;

        const char *buf;
        bool need_free = false;

        ap = argparser_create("count", COUNT_VERSION);
        if (!ap) {
                fprintf(stderr, "Failed to create argparser\n");
                exit(1);
        }

        argparser_add0(ap, &help, "h", "help", "show this help message and exit", __acb_help, opt_none);
        argparser_add0(ap, &version, "version", NULL, "show current version", __acb_version, opt_none);
        argparser_add0(ap, &c, "c", NULL, "character count", NULL, opt_none);
        argparser_add0(ap, &l, "l", NULL, "line count", NULL, opt_none);
        argparser_add1(ap, &f, "f", NULL, "read file contents", NULL, opt_reqval);

        if (argparser_run(ap, argc, argv) != 0) {
                fprintf(stderr, "%s\n", argparser_error(ap));
                argparser_free(ap);
                return -1;
        }

        if ((buf = strread(ap, f, &need_free)) == NULL) {
                fprintf(stderr, "%s\n", strerror(errno));
                return -1;
        }

        /* calculate lines */
        if (l) {
                printf("%zu\n", linec(buf));
                goto end;
        }

        /* default calculates word count */
        printf("%zu\n", wordc(buf, c != NULL));

end:
        if (need_free)
                free((void *) buf);

        argparser_free(ap);

        return 0;
}
