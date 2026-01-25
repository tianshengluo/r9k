#include "log.h"

#include <stdio.h>
#include <stdarg.h>

void _logger_record(const char *level, const char *fmt, ...)
{
        char msg[512];

        va_list va;
        va_start(va, fmt);
        vsnprintf(msg, sizeof(msg), fmt, va);
        va_end(va);

        printf("[%-5s] %s", level, msg);
}