/*
    This file is part of Circus.

    Circus is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License.

    Circus is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Circus.  If not, see <http://www.gnu.org/licenses/>.

    Copyright © 2015-2016 Cyril Adrian <cyril.adrian@gmail.com>
*/

#ifndef __CIRCUS_LOG_H
#define __CIRCUS_LOG_H

#include <cad_stream.h>

#include <circus.h>

typedef struct circus_log_s circus_log_t;

typedef enum {
   LOG_ERROR=0,
   LOG_WARNING,
   LOG_INFO,
   LOG_DEBUG,
   LOG_PII,
   __LOG_MAX
} log_level_t;

#define DEFAULT_FORMAT "%X %Y-%M-%D %h:%m:%s.%u [%T] %O:%G\n"

typedef void (*circus_log_set_log_fn)(circus_log_t *this, const char *module, log_level_t max_level);
typedef int (*circus_log_is_log_fn)(circus_log_t *this, const char *module, log_level_t level);
typedef void (*circus_log_set_format_fn)(circus_log_t *this, const char *format);
typedef cad_output_stream_t *(*circus_log_stream_fn)(circus_log_t *this, const char *module, log_level_t level);
typedef void (*circus_log_close_fn)(circus_log_t *this);
typedef void (*circus_log_free_fn)(circus_log_t *this);

struct circus_log_s {
   circus_log_set_log_fn set_log;
   circus_log_is_log_fn is_log;
   circus_log_set_format_fn set_format;
   circus_log_stream_fn stream;
   circus_log_close_fn close;
   circus_log_free_fn free;
};

__PUBLIC__ circus_log_t *circus_new_log_file(cad_memory_t memory, const char *filename, log_level_t max_level);
__PUBLIC__ circus_log_t *circus_new_log_file_descriptor(cad_memory_t memory, log_level_t max_level, int fd);

__PUBLIC__ void logger_error(circus_log_t *logger, const char *module, int line, const char *format, ...) __attribute__((format(printf, 4, 5)));
__PUBLIC__ void logger_warning(circus_log_t *logger, const char *module, int line, const char *format, ...) __attribute__((format(printf, 4, 5)));
__PUBLIC__ void logger_info(circus_log_t *logger, const char *module, int line, const char *format, ...) __attribute__((format(printf, 4, 5)));
__PUBLIC__ void logger_debug(circus_log_t *logger, const char *module, int line, const char *format, ...) __attribute__((format(printf, 4, 5)));
__PUBLIC__ void logger_pii(circus_log_t *logger, const char *module, int line, const char *format, ...) __attribute__((format(printf, 4, 5)));

#define log_error(logger, format, arg...) logger_error((logger), __FILE__, __LINE__, (format) , ##arg)
#define log_warning(logger, format, arg...) logger_warning((logger), __FILE__, __LINE__, (format) , ##arg)
#define log_info(logger, format, arg...) logger_info((logger), __FILE__, __LINE__, (format) , ##arg)
#define log_debug(logger, format, arg...) logger_debug((logger), __FILE__, __LINE__, (format) , ##arg)
#define log_pii(logger, format, arg...) logger_pii((logger), __FILE__, __LINE__, (format) , ##arg)

#define log_is_error(logger) ((logger)->is_log((logger), __FILE__, LOG_ERROR))
#define log_is_warning(logger) ((logger)->is_log((logger), __FILE__, LOG_WARNING))
#define log_is_info(logger) ((logger)->is_log((logger), __FILE__, LOG_INFO))
#define log_is_debug(logger) ((logger)->is_log((logger), __FILE__, LOG_DEBUG))
#define log_is_pii(logger) ((logger)->is_log((logger), __FILE__, LOG_PII))

#endif /* __CIRCUS_LOG_H */
