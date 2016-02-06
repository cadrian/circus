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
   __LOG_MAX
} log_level_t;

#define DEFAULT_FORMAT "%Y-%M-%D %h:%m:%s.%u [%T] %O: %G\n"

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

__PUBLIC__ void log_error(circus_log_t *logger, const char *module, const char *format, ...) __attribute__((format(printf, 3, 4)));
__PUBLIC__ void log_warning(circus_log_t *logger, const char *module, const char *format, ...) __attribute__((format(printf, 3, 4)));
__PUBLIC__ void log_info(circus_log_t *logger, const char *module, const char *format, ...) __attribute__((format(printf, 3, 4)));
__PUBLIC__ void log_debug(circus_log_t *logger, const char *module, const char *format, ...) __attribute__((format(printf, 3, 4)));

#endif /* __CIRCUS_LOG_H */
