#include "log.h"

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <r9k/string.h>

#define COLOR_RESET   "\x1b[0m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"

static const char *getcolor(const char *level)
{
        if (streq(level, "ERROR") || streq(level, "FATAL"))
                return COLOR_RED;

        if (streq(level, "WARN"))
                return COLOR_YELLOW;

        if (streq(level, "INFO"))
                return COLOR_GREEN;

        return COLOR_YELLOW;
}

void _logger_record(const char *level, const char *fmt, ...)
{
        char strtime[128];
        char msg[4096];
        struct timeval tv;
        time_t now = time(NULL);
        struct tm *time_info;

        gettimeofday(&tv, NULL);

        va_list va;
        va_start(va, fmt);
        vsnprintf(msg, sizeof(msg), fmt, va);
        va_end(va);

        time_info = localtime(&now);
        strftime(strtime, sizeof(strtime), "%Y-%m-%d %H:%M:%S", time_info);

        printf("%s+%03ld [%d] %s%s%s - %s",
                strtime,
                tv.tv_usec / 1000,
                getpid(),
                getcolor(level),
                level,
                COLOR_RESET,
                msg);
}
