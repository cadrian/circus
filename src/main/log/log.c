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

#define DEFAULT_FORMAT "%Y-%M-%D %h:%m:%s %F:%L [%T] %O: %G\n"

static int default_level = LOG_INFO;
static cad_hash_t *module_levels = NULL;

static uv_file fd;
static uv_stream_t *log_handle = NULL;
static int64_t offset = 0;

const char *log_format = DEFAULT_FORMAT;

static int format_log(char *buf, const int buflen, const char *format, struct tm *tm, const char *file, int line, const char *tag, const char *module, const char *message) {
   int result = 0;
   int state = 0;
   const char *f;
   for (f = format; *f; f++) {
      switch(state) {
      case 0:
         if (*f == '%') {
            state = 1;
         } else {
            if (result < buflen) {
               buf[result] = *f;
            }
            result++;
         }
         break;
      case 1:
         switch(*f) {
         case '%':
            if (result < buflen) {
               buf[result] = '%';
            }
            result++;
            break;
         case 'D':
            if (result < buflen) {
               result += snprintf(buf + result, buflen - result, "%02d", tm->tm_mday);
            } else {
               result += 2;
            }
            break;
         case 'M':
            if (result < buflen) {
               result += snprintf(buf + result, buflen - result, "%02d", tm->tm_mon);
            } else {
               result += 2;
            }
            break;
         case 'Y':
            if (result < buflen) {
               result += snprintf(buf + result, buflen - result, "%04d", tm->tm_year + 1900);
            } else {
               result += 4;
            }
            break;
         case 'h':
            if (result < buflen) {
               result += snprintf(buf + result, buflen - result, "%02d", tm->tm_hour);
            } else {
               result += 2;
            }
            break;
         case 'm':
            if (result < buflen) {
               result += snprintf(buf + result, buflen - result, "%02d", tm->tm_min);
            } else {
               result += 2;
            }
            break;
         case 's':
            if (result < buflen) {
               result += snprintf(buf + result, buflen - result, "%02d", tm->tm_sec);
            } else {
               result += 2;
            }
            break;
         case 'F':
            if (result < buflen) {
               result += snprintf(buf + result, buflen - result, "%s", file);
            } else {
               result += strlen(file);
            }
            break;
         case 'L':
            if (result < buflen) {
               result += snprintf(buf + result, buflen - result, "%d", line);
            } else {
               result += snprintf("", 0, "%d", line);
            }
            break;
         case 'T':
            if (result < buflen) {
               result += snprintf(buf + result, buflen - result, "%7s", tag);
            } else {
               result += 7;
            }
            break;
         case 'O':
            if (result < buflen) {
               result += snprintf(buf + result, buflen - result, "%s", module);
            } else {
               result += strlen(module);
            }
            break;
         case 'G':
            if (result < buflen) {
               result += snprintf(buf + result, buflen - result, "%s", message);
            } else {
               result += strlen(message);
            }
            break;
         default:
            fprintf(stderr, "Invalid flag %%%c\n", *f);
            crash();
         }
         state = 0;
      }
   }
   if (result < buflen) {
      buf[result] = '\0';
   }
   return result;
}

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

void circus_set_format(const char *format) {
   log_format = format;
}

static void start_log_tty(void) {
   uv_tty_t *tty;
   uv_pipe_t *pipe;
   uv_handle_type handle_type = uv_guess_handle(1);
   switch(handle_type) {
   case UV_TTY:
      tty = malloc(sizeof(uv_tty_t));
      uv_tty_init(uv_default_loop(), tty, 1, 0);
      uv_tty_set_mode(tty, UV_TTY_MODE_NORMAL);
      log_handle = (uv_stream_t*)tty;
      break;
   case UV_NAMED_PIPE:
      pipe = malloc(sizeof(uv_pipe_t));
      uv_pipe_init(uv_default_loop(), pipe, 0);
      uv_pipe_open(pipe, 1);
      log_handle = (uv_stream_t*)pipe;
      break;
   case UV_FILE:
      log_handle = malloc(sizeof(uv_stream_t));
      log_handle->type = UV_FILE;
      offset = 0;
      break;
   default:
      fprintf(stderr, "handle_type=%d not handled...\n", handle_type);
      crash();
   }
   fd = 1;
}

void circus_start_log(const char *filename) {
   circus_log_end();
   if (filename) {
      uv_fs_t *req = malloc(sizeof(uv_fs_t));
      assert(req);
      fd = uv_fs_open(uv_default_loop(), req, filename, O_WRONLY | O_CREAT | O_APPEND, 0600, NULL);
      if (fd > 0) {
         uv_pipe_t *pipe = malloc(sizeof(uv_pipe_t));
         uv_pipe_init(uv_default_loop(), pipe, 0);
         uv_pipe_open(pipe, fd);
         log_handle = (uv_stream_t*)pipe;
      } else {
         start_log_tty();
      }
      free(req);
   } else {
      start_log_tty();
   }
}

typedef struct write_req_s write_req_t;
struct write_req_s {
   union {
      uv_write_t stream;
      uv_fs_t fs;
   } req;
   uv_buf_t buf;
   void (*cleanup)(write_req_t*);
};

static void cleanup_stream(write_req_t *req) {
   free(req->buf.base);
}

static void cleanup_fs(write_req_t *req) {
   free(req->buf.base);
   uv_fs_req_cleanup(&req->req.fs);
}

static void on_write(write_req_t* req, int status) {
   req->cleanup(req);
   free(req);
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
   int n;
   char *logline = NULL;

   va_start(args, format);
   message = vszprintf(format, args);
   assert(message != NULL);
   va_end(args);

   n = format_log("", 0, log_format, tm, file, line, tag, module, message);
   logline = malloc(n + 1);
   assert(logline != NULL);
   format_log(logline, n + 1, log_format, tm, file, line, tag, module, message);
   free(message);

   write_req_t *req = malloc(sizeof(write_req_t));
   int len = strlen(logline);
   req->buf.base = logline;
   req->buf.len = len;
   switch(log_handle->type) {
   case UV_TTY:
   case UV_NAMED_PIPE:
      req->cleanup = cleanup_stream;
      uv_write(&req->req.stream, log_handle, &req->buf, 1, (uv_write_cb)on_write);
      break;
   case UV_FILE:
      req->cleanup = cleanup_fs;
      uv_fs_write(uv_default_loop(), &req->req.fs, fd, &req->buf, 1, offset, (uv_fs_cb)on_write);
      offset += len;
      break;
   default:
      crash();
   }
}

void circus_log_end(void) {
   uv_fs_t* req;
   if (log_handle) {
      switch(log_handle->type) {
      case UV_TTY:
         uv_tty_reset_mode();
         break;
      case UV_NAMED_PIPE:
         req = malloc(sizeof(uv_fs_t));
         assert(req);
         uv_fs_close(uv_default_loop(), req, fd, NULL);
         free(req);
         break;
      case UV_FILE:
         break;
      default:
         crash();
      }
      free(log_handle);
      log_handle = NULL;
   }
}
