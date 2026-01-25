#ifndef LOGGER_H_
#define LOGGER_H_

#include <string.h>
#include <errno.h>

void _logger_record(const char *level, const char *fmt, ...); // NOLINT(*-reserved-identifier)

#define LOG_INFO(fmt, ...)  _logger_record("INFO", fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  _logger_record("WARN", fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) _logger_record("ERROR", fmt, ##__VA_ARGS__)

#ifndef N_DEBUG
#define LOG_DEBUG(fmt, ...) _logger_record("DEBUG", fmt, ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...)
#endif /* N_DEBUG */

#define syserr strerror(errno)

#endif /* LOGGER_H_ */
