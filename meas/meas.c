/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 *
 * siz - A command-line tool for calculation file sizes and string lengths
 *
 * Supports file size calculation (with human-readable output)
 * and string length counting (both byte length and UTF-8 character count)
 */
#include <stdio.h>
#include <string.h>
#include <argparse.h>

#define MEAS_VERSION "1.0.0"

static struct option options[] = {
        { 'a', "a1", no_arguments, "1" },
        { 'b', "b2", required_argument, "2" },
        { 'c', "c3", no_arguments, "3" },
        { 0 },
};

static size_t utf8len(const char *str)
{
        size_t len = 0;

        while (*str)
                len += (*str++ & 0xc0) != 0x80;

        return len;
}

static ssize_t filesize(const char *path)
{
        FILE *fp;
        long int size;

        fp = fopen(path, "rb");
        if (!fp)
                return -1;

        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        fclose(fp);

        return size;
}

static double human_size(size_t size, const char **unit)
{
        /* K Byte */
        if (size <= (1024 * 1024)) {
                *unit = "K";
                return (double) size / 1024;
        }

        /* M Byte */
        if (size <= (1024 * 1024 * 1024)) {
                *unit = "M";
                return (double) size / 1024 / 1024;
        }

        /* G Byte */
        *unit = "G";
        return (double) size / 1024 / 1024 / 1024;
}

int main(int argc, char **argv)
{
        argparse_t *ap;
        ap = argparse_parse(options, argc, argv);

        if (!ap) {
                fprintf(stderr, "error: %s\n", argparse_error());
                exit(EXIT_FAILURE);
        }

        if (argparse_has(ap, "a")) {
                printf("a: exist\n");
        }

        if (argparse_has(ap, "b")) {
                printf("b: exist\n");
                printf("b value: %s\n", argparse_val(ap, "b"));
        }

        if (argparse_has(ap, "b")) {
                printf("c: exist\n");
        }

        size_t count;
        const char **args = argparse_args(ap, &count);
        for (int i = 0; i < count; i++)
                printf("arg[%d]: %s\n", i, args[i]);

        argparse_free(ap);

        return 0;
}
