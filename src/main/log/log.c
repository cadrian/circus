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

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include <cad_shared.h>
#include <cad_hash.h>
#include <cad_array.h>

#include <uv.h>

#include "log.h"

typedef struct log_line_s log_line_t;
struct log_line_s {
   log_line_t *next;
   time_t date;
   const char *file;
   int line;
   const char *tag;
   const char *module;
   char buffer[0];
};

static int default_level = LOG_INFO;
static cad_hash_t *module_levels = NULL;

void circus_set_log(const char *module, int max_level) {
   int *level;
   if (!module) {
      default_level = max_level;
   } else {
      if (!module_levels) {
         module_levels = cad_new_hash(stdlib_memory, cad_hash_strings);
      }
      level = module_levels->get(module_levels, module);
      if (level == NULL) {
         level = malloc(sizeof(int));
         module_levels->set(module_levels, module, level);
      }
      *level = max_level;
   }
}

int circus_is_log(int level, const char *module) {
   int max_level = default_level;
   if (module) {
      if (module_levels) {
         int *level = module_levels->get(module_levels, module);
         if (level) {
            max_level = *level;
         }
      }
   }
   return level <= max_level;
}

static uv_poll_t *log_handle = NULL;
static log_line_t *first_line = NULL;
static log_line_t *last_line = NULL;

void on_log_writable(uv_poll_t* handle, int status, int events) {
   log_line_t *log_line = first_line;
   log_line_t *next;
   struct tm *tm;
   if (status == 0) {
      first_line = last_line = NULL;
      while (log_line) {
         next = log_line->next;
         tm = localtime(&(log_line->date));
         printf("%02d/%02d/%04d %02d:%02d:%02d %s:%d [%7s] %s: %s\n",
                tm->tm_mday, tm->tm_mon, tm->tm_year,
                tm->tm_hour, tm->tm_min, tm->tm_sec,
                log_line->file, log_line->line,
                log_line->tag, log_line->module, log_line->buffer);
         free(log_line);
         log_line = next;
      }
   }
}

static void circus_init_log(void) {
   log_handle = malloc(sizeof(uv_poll_t));
   uv_poll_init(uv_default_loop(), log_handle, 1); // 1 is stdout
   uv_poll_start(log_handle, UV_WRITABLE, on_log_writable);
}

void circus_log(const char *file, int line, const char *tag, const char *module, const char *format, ...) {
   static int init=0;

   va_list args, args2;
   int n;
   log_line_t *log_line;

   if (!init) {
      init = 1;
      circus_init_log();
   }

   va_start(args, format);
   va_copy(args2, args);

   n = vsnprintf("", 0, format, args);
   log_line = malloc(sizeof(log_line_t) + n+1);
   log_line->next = NULL;
   log_line->date = time(NULL);
   log_line->file = file;
   log_line->line = line;
   log_line->tag = tag;
   log_line->module = module;
   log_line->buffer[0] = '\0';
   if (n) {
      vsnprintf(log_line->buffer, n+1, format, args2);
   }

   va_end(args);
   va_end(args2);

   if (last_line) {
      last_line->next = log_line;
      last_line = log_line;
   } else {
      first_line = last_line = log_line;
   }
}

void circus_log_end(void) {
   if (log_handle) {
      uv_poll_stop(log_handle);
      free(log_handle);
   }
}
