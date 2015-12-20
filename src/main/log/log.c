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

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include <cad_shared.h>
#include <cad_hash.h>
#include <cad_array.h>

#include <uv.h>

#include "circus.h"
#include "log.h"

static int default_level = LOG_INFO;
static cad_hash_t *module_levels = NULL;

static uv_tty_t tty;
static uv_pipe_t pipe;
static uv_file fd;
static uv_stream_t *log_handle = NULL;

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
         assert(level);
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

static void start_log_tty(void) {
   uv_tty_init(uv_default_loop(), &tty, 1, 0);
   uv_tty_set_mode(&tty, UV_TTY_MODE_NORMAL);
   log_handle = (uv_stream_t*)&tty;
}

void circus_start_log(const char *filename) {
   circus_log_end();
   if (filename) {
      uv_fs_t *req = malloc(sizeof(uv_fs_t));
      assert(req);
      fd = uv_fs_open(uv_default_loop(), req, filename, O_WRONLY | O_CREAT | O_APPEND, 0600, NULL);
      if (fd > 0) {
         uv_pipe_init(uv_default_loop(), &pipe, 0);
         uv_pipe_open(&pipe, fd);
         log_handle = (uv_stream_t*)&pipe;
      } else {
         start_log_tty();
      }
      free(req);
   } else {
      start_log_tty();
   }
}

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;

static void on_write(uv_write_t* req, int status) {
   write_req_t *wreq = (write_req_t*) req;
   free(wreq->buf.base);
   free(wreq);
}

void circus_log(const char *file, int line, const char *tag, const char *module, const char *format, ...) {
   assert(log_handle != NULL);
   assert(file != NULL);
   assert(line > 0);
   assert(tag != NULL);
   assert(module != NULL);
   assert(format != NULL);

   va_list args;
   time_t date = time(NULL);
   struct tm *tm = localtime(&date);
   char *message = NULL;
   char *logline = NULL;

   va_start(args, format);
   message = vszprintf(format, args);
   assert(message != NULL);
   va_end(args);

   logline = szprintf("%02d/%02d/%04d %02d:%02d:%02d %s:%d [%7s] %s: %s\n",
                      tm->tm_mday, tm->tm_mon, tm->tm_year,
                      tm->tm_hour, tm->tm_min, tm->tm_sec,
                      file, line, tag, module, message);
   assert(logline != NULL);
   free(message);

   write_req_t *req = malloc(sizeof(write_req_t));
   req->buf.base = logline;
   req->buf.len = strlen(logline);
   uv_write(&req->req, log_handle, &req->buf, 1, on_write);
}

void circus_log_end(void) {
   if (log_handle) {
      if (log_handle == (uv_stream_t*)&tty) {
         uv_tty_reset_mode();
      } else {
         assert(log_handle == (uv_stream_t*)&pipe);
         uv_fs_t* req = malloc(sizeof(uv_fs_t));
         assert(req);
         uv_fs_close(uv_default_loop(), req, fd, NULL);
         free(req);
      }
      log_handle = NULL;
   }
}
