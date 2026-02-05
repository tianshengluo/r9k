#include "log.h"

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

void _logger_record(const char *level, const char *fmt, ...)
{
        char strtime[128];
        char msg[4096];
        time_t now = time(NULL);
        struct tm *time_info;

        va_list va;
        va_start(va, fmt);
        vsnprintf(msg, sizeof(msg), fmt, va);
        va_end(va);

        time_info = localtime(&now);
        strftime(strtime, sizeof(strtime), "%Y-%m-%d %H:%M:%S", time_info);

        printf("%s %s %s", strtime, level, msg);
}
