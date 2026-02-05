#ifndef LOGGER_H_
#define LOGGER_H_

#include <string.h>
#include <errno.h>
#include <r9k/compiler_attrs.h>

__attr_printf(2, 3)
void _logger_record(const char *level, const char *fmt, ...);

#define log_info(fmt, ...)  _logger_record("INFO", fmt, ##__VA_ARGS__)
#define log_warn(fmt, ...)  _logger_record("WARN", fmt, ##__VA_ARGS__)
#define log_error(fmt, ...) _logger_record("ERROR", fmt, ##__VA_ARGS__)
#define log_fatal(fmt, ...) _logger_record("FATAL", fmt, ##__VA_ARGS__)

#ifndef N_DEBUG
#define log_debug(fmt, ...) _logger_record("DEBUG", fmt, ##__VA_ARGS__)
#else
#define log_debug(fmt, ...)
#endif /* N_DEBUG */

#define syserr strerror(errno)

#endif /* LOGGER_H_ */
