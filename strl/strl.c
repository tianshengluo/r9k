/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <r9k/argparser.h>
#include <r9k/typedefs.h>
#include <r9k/string.h>

#define BUFSIZE 262144 /* 256kb */

struct worker_arg_t
{
        const char *filename;
        struct option *ischr;
        ssize_t ret;
        int err;
};

static size_t utf8len(const char *str)
{
        size_t len = 0;

        while (*str) {
                unsigned char c = (unsigned char) *str;
                str++;
                if ((c & 0xC0) != 0x80)
                        len++;
        }

        return len;
}

static size_t stdin_count(struct option *ischr)
{
        char buf[BUFSIZE + 1];
        ssize_t n;
        size_t count = 0;

        while (1) {
                n = read(STDIN_FILENO, buf, BUFSIZE);
                if (n <= 0)
                        break;
                buf[n] = '\0';
                count += ischr ? utf8len(buf) : strlen(buf);
        }

        return count;
}

static ssize_t file_count(const char *filename, struct option *ischr, int *err)
{
        char buf[BUFSIZE + 1];
        size_t total = 0;
        ssize_t n;

        FILE *fp = fopen(filename, "r");
        if (!fp) {
                *err = errno;
                return -1;
        }

        while ((n = fread(buf, 1, BUFSIZE, fp)) > 0) {
                if (n <= 0)
                        break;
                buf[n] = '\0';
                total += ischr ? utf8len(buf) : strlen(buf);
        }

        if (ferror(fp)) {
                fclose(fp);
                *err = errno;
                return -1;
        }

        fclose(fp);
        return (ssize_t) total;
}

static void *file_count_worker(void *_arg)
{
        struct worker_arg_t *arg = _arg;
        arg->ret = file_count(arg->filename, arg->ischr, &arg->err);
        return NULL;
}

static void process_files(struct option *files, struct option *ischr)
{
        if (!files)
                return;

        pthread_t threads[files->nval];
        struct worker_arg_t args[files->nval];
        size_t total = 0;

        /* create pthreads */
        for (uint32_t i = 0; i < files->nval; i++) {
                args[i].filename = files->vals[i];
                args[i].ischr = ischr;
                args[i].ret = 0;
                pthread_create(&threads[i], NULL, file_count_worker, &args[i]);
        }

        /* join threads */
        for (uint32_t i = 0; i < files->nval; i++) {
                pthread_join(threads[i], NULL);

                if (args[i].ret == -1)
                        die("ERROR read file: %s\n", strerror(args[i].err));

                printf("%s %zu\n", args[i].filename, args[i].ret);
                total += args[i].ret;
        }

        printf("total %zu\n", total);

        exit(0);
}

static void process_normal(struct argparser *ap, struct option *ischr)
{
        if (argparser_count(ap) > 0) {
                const char *buf = argparser_val(ap, 0);
                printf("%zu\n", ischr ? utf8len(buf) : strlen(buf));
        } else {
                printf("%zu\n", stdin_count(ischr));
        }
        exit(0);
}

int main(int argc, char* argv[])
{
        struct argparser *ap;
        struct option *help;
        struct option *version;
        struct option *ischr;
        struct option *files;

        ap = argparser_create("strl", "1.0.0");
        if (!ap)
                die("argparser initialize failed");

        argparser_add0(ap, &help, "h", "help", "show this help message.", ACB_HELP, 0);
        argparser_add0(ap, &version, "version", NULL, "show current version.", ACB_VERSION, 0);
        argparser_add0(ap, &ischr, "c", NULL, "count characters by unicode.", NULL, 0);
        argparser_addn(ap, &files, "f", NULL, 128, "count files.", NULL, OPT_REQUIRED);

        if (argparser_run(ap, argc, argv) != 0)
                die("%s", argparser_error(ap));

        process_files(files, ischr);
        process_normal(ap, ischr);

        argparser_free(ap);

        return 0;
}