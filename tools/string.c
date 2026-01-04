/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Varketh Nockrath
 */
#include <r9k/string.h>
#include <ctype.h>

int is_str_blank(const char *str)
{
        if (!str)
                return 1;

        while (*str) {
                if (!isspace(*str))
                        return 0;
                str++;
        }

        return 0;
}

char *trim_start(char *str)
{
        if (!str)
                return NULL;

        char *start = str;

        while (isspace((unsigned char) *start))
                start++;

        return start;
}

char *trim_end(char *str)
{
        if (!str)
                return NULL;

        char *end = str;

        while (*end)
                end++;

        while (end > str && isspace((unsigned char) *(end - 1)))
                end--;

        *end = '\0';

        return str;
}

char *trim(char *str)
{
        if (!str)
                return NULL;

        trim_end(str);

        return trim_start(str);
}