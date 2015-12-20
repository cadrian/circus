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
*/
#ifndef __CIRCUS_LOG_H
#define __CIRCUS_LOG_H

typedef enum {
   LOG_ERROR=0,
   LOG_WARNING,
   LOG_INFO,
   LOG_DEBUG,
} log_level_t;

void circus_start_log(const char *filename);
void circus_set_log(const char *module, int max_level);
int circus_is_log(int level, const char *module);
void circus_log(const char *file, int line, const char *tag, const char *module, const char *format, ...) __attribute__((format(printf, 5, 6)));
void circus_log_end(void);

#define log_error(module, format, ...)                                  \
   do {                                                                 \
      if (circus_is_log(LOG_ERROR, (module)))                           \
         circus_log(__FILE__, __LINE__, "ERROR", (module), (format), __VA_ARGS__); \
   } while (0)

#define log_warning(module, format, ...)                                \
   do {                                                                 \
      if (circus_is_log(LOG_WARNING, (module)))                         \
         circus_log(__FILE__, __LINE__, "WARNING", (module), (format), __VA_ARGS__); \
   } while (0)

#define log_info(module, format, ...)                                   \
   do {                                                                 \
      if (circus_is_log(LOG_INFO, (module)))                            \
         circus_log(__FILE__, __LINE__, "INFO", (module), (format), __VA_ARGS__); \
   } while (0)

#define log_debug(module, format, ...)                                  \
   do {                                                                 \
      if (circus_is_log(LOG_DEBUG, (module)))                           \
         circus_log(__FILE__, __LINE__, "DEBUG", (module), (format), __VA_ARGS__); \
   } while (0)

#endif /* __CIRCUS_LOG_H */
