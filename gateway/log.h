#ifndef LOGGER_H_
#define LOGGER_H_

#include <string.h>
#include <errno.h>

void _logger_record(const char *level, const char *fmt, ...); // NOLINT(*-reserved-identifier)

#define logger_info(fmt, ...)  _logger_record("INFO", fmt, ##__VA_ARGS__)
#define logger_warn(fmt, ...)  _logger_record("WARN", fmt, ##__VA_ARGS__)
#define logger_error(fmt, ...) _logger_record("ERROR", fmt, ##__VA_ARGS__)

#ifndef N_DEBUG
#define logger_debug(fmt, ...) _logger_record("DEBUG", fmt, ##__VA_ARGS__)
#else
#define logger_debug(fmt, ...)
#endif /* N_DEBUG */

#define syserr strerror(errno)

#endif /* LOGGER_H_ */
