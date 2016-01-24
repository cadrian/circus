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
#include <sys/time.h>

#include <cad_shared.h>
#include <cad_hash.h>
#include <cad_array.h>

#include <uv.h>

#include <circus_log.h>

static const char* level_tag[] = {"ERROR", "WARNING", "INFO", "DEBUG"};

static int format_log(char *buf, const int buflen, const char *format, const struct timeval *tv, const char *tag, const char *module, const char *message) {
   int result = 0;
   int state = 0;
   struct tm *tm = localtime(&(tv->tv_sec));
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
         case 'u':
            if (result < buflen) {
               result += snprintf(buf + result, buflen - result, "%03d", (int)(tv->tv_usec / 1000));
            } else {
               result += 3;
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

/* ---------------------------------------------------------------- */

typedef struct write_req_s write_req_t;
struct write_req_s {
   union {
      uv_write_t stream;
      uv_fs_t fs;
   } req;
   cad_memory_t memory;
   uv_buf_t buf;
   void (*cleanup)(write_req_t*);
};

static void cleanup_stream(write_req_t *req) {
   req->memory.free(req->buf.base);
}

static void cleanup_fs(write_req_t *req) {
   req->memory.free(req->buf.base);
   uv_fs_req_cleanup(&req->req.fs);
}

static void on_write(write_req_t* req, int UNUSED(status)) {
   req->cleanup(req);
   req->memory.free(req);
}

/* ---------------------------------------------------------------- */

typedef struct uv_logger_s uv_logger_t;
typedef void (*logger_free_fn)(uv_logger_t *logger);
typedef void (*logger_write_fn)(uv_logger_t *logger, write_req_t *req);
typedef void (*logger_flush_fn)(uv_logger_t *logger);
typedef struct {
   logger_free_fn  free ;
   logger_write_fn write;
   logger_flush_fn flush;
} uv_logger_fn;

struct uv_logger_s {
   uv_logger_fn fn;
   cad_memory_t memory;
   uv_stream_t *log_handle;
   uv_file fd;
   int64_t offset;
   char *format;
};

static void file_free(uv_logger_t *this) {
   assert(this->log_handle == NULL);
   this->memory.free(this);
}

static void file_write(uv_logger_t *this, write_req_t *req) {
   int64_t offset = this->offset;
   req->cleanup = cleanup_fs;
   this->offset += req->buf.len;
   uv_fs_write(uv_default_loop(), &req->req.fs, this->fd, &req->buf, 1, offset, (uv_fs_cb)on_write);
}

static void file_flush(uv_logger_t *this) {
   uv_fs_t req;
   uv_fs_fdatasync(uv_default_loop(), &req, this->fd, NULL);
   // TODO: be sure to wait that all the data has been written before
   // flushing. This will need some clever use of this->offset
   // (expected offset) and a new counter in the on_write callback
   // (actual written offset) + using a uv_mutex_t and uv_cond_t
   // barrier
}

/**
 * FILE is used when stdout was redirected
 */
static uv_logger_fn logger_file_fn = {
   (logger_free_fn)file_free,
   (logger_write_fn)file_write,
   (logger_flush_fn)file_flush,
};

static void tty_free(uv_logger_t *this) {
   uv_tty_reset_mode();
   this->memory.free(this->log_handle);
   this->memory.free(this);
}

static void tty_write(uv_logger_t *this, write_req_t *req) {
   req->cleanup = cleanup_stream;
   uv_write(&req->req.stream, this->log_handle, &req->buf, 1, (uv_write_cb)on_write);
}

static void tty_flush(uv_logger_t *UNUSED(this)) {
   // TODO: just some kind of barrier too (see log_flush_file for explanations)
}

/**
 * TTY is used for normal tty-attached stdout
 */
static uv_logger_fn logger_tty_fn = {
   (logger_free_fn)tty_free,
   (logger_write_fn)tty_write,
   (logger_flush_fn)tty_flush,
};

static void pipe_free(uv_logger_t *this) {
   uv_fs_t req;
   uv_fs_close(uv_default_loop(), &req, this->fd, NULL);
   this->memory.free(this->log_handle);
   this->memory.free(this);
}

static void pipe_write(uv_logger_t *this, write_req_t *req) {
   req->cleanup = cleanup_stream;
   uv_write(&req->req.stream, this->log_handle, &req->buf, 1, (uv_write_cb)on_write);
}

static void pipe_flush(uv_logger_t *UNUSED(this)) {
   // TODO: just some kind of barrier too (see log_flush_file for explanations)
}

/**
 * PIPE is used for logs written to file (not for stdout, see FILE)
 */
static uv_logger_fn logger_pipe_fn = {
   (logger_free_fn)pipe_free,
   (logger_write_fn)pipe_write,
   (logger_flush_fn)pipe_flush,
};

static uv_logger_t *new_logger_file(cad_memory_t memory, const char *filename) {
   assert(filename != NULL);

   uv_logger_t *result = NULL;
   uv_fs_t req;
   uv_file fd = uv_fs_open(uv_default_loop(), &req, filename, O_WRONLY | O_CREAT | O_APPEND, 0600, NULL);
   if (fd >= 0) {
      result = memory.malloc(sizeof(uv_logger_t));
      uv_pipe_t *pipe = memory.malloc(sizeof(uv_pipe_t));
      uv_pipe_init(uv_default_loop(), pipe, 0);
      uv_pipe_open(pipe, fd);

      result->fn = logger_pipe_fn;
      result->memory = memory;
      result->log_handle = (uv_stream_t*)pipe;
      result->fd = fd;
      result->offset = 0;
      result->format = DEFAULT_FORMAT;
   }

   return result;
}

static uv_logger_t *new_logger_stdout(cad_memory_t memory) {
   uv_logger_t *result = memory.malloc(sizeof(uv_logger_t));
   uv_tty_t *tty;
   uv_pipe_t *pipe;
   uv_handle_type handle_type = uv_guess_handle(1);
   result->memory = memory;
   result->fd = 1;
   result->offset = 0;
   switch(handle_type) {
   case UV_TTY:
      tty = memory.malloc(sizeof(uv_tty_t));
      uv_tty_init(uv_default_loop(), tty, 1, 0);
      uv_tty_set_mode(tty, UV_TTY_MODE_NORMAL);
      result->fn = logger_tty_fn;
      result->log_handle = (uv_stream_t*)tty;
      break;
   case UV_NAMED_PIPE:
      pipe = memory.malloc(sizeof(uv_pipe_t));
      uv_pipe_init(uv_default_loop(), pipe, 0);
      uv_pipe_open(pipe, 1);
      result->fn = logger_pipe_fn;
      result->log_handle = (uv_stream_t*)pipe;
      break;
   case UV_FILE:
      result->fn = logger_file_fn;
      result->log_handle = NULL;
      break;
   default:
      fprintf(stderr, "handle_type=%d not handled...\n", handle_type);
      crash();
   }
   result->format = DEFAULT_FORMAT;
   return result;
}

/* ---------------------------------------------------------------- */

typedef struct {
   cad_output_stream_t fn;
   cad_memory_t memory;
   uv_logger_t *logger;
   const char *module;
   const char *tag;
} log_file_output_stream;

static void log_free_stream(log_file_output_stream *this) {
   this->memory.free(this);
}

static void log_vput_stream(log_file_output_stream *this, const char *format, va_list args) {
   write_req_t *req;
   struct timeval tv;
   char *message = NULL;
   int n, e;
   char *logline = NULL;

   e = gettimeofday(&tv, NULL);
   assert(e == 0);

   message = vszprintf(this->memory, NULL, format, args);
   assert(message != NULL);

   n = format_log("", 0, this->logger->format, &tv, this->tag, this->module, message);
   logline = this->memory.malloc(n + 1);
   assert(logline != NULL);
   format_log(logline, n + 1, this->logger->format, &tv, this->tag, this->module, message);
   this->memory.free(message);

   req = this->memory.malloc(sizeof(write_req_t));
   assert(req != NULL);
   req->buf.base = logline;
   req->buf.len = n;
   req->memory = this->memory;

   this->logger->fn.write(this->logger, req);
}

static void log_put_stream(log_file_output_stream *this, const char *format, ...) {
   va_list args;
   va_start(args, format);
   log_vput_stream(this, format, args);
   va_end(args);
}

static void log_flush_stream(log_file_output_stream *this) {
   this->logger->fn.flush(this->logger);
}

cad_output_stream_t log_stream_fn = {
   .free  = (cad_output_stream_free_fn)log_free_stream,
   .put   = (cad_output_stream_put_fn)log_put_stream,
   .vput  = (cad_output_stream_vput_fn)log_vput_stream,
   .flush = (cad_output_stream_flush_fn)log_flush_stream,
};

static cad_output_stream_t *new_log_stream(cad_memory_t memory, uv_logger_t *logger, const char *module, const char *tag) {
   log_file_output_stream *result = memory.malloc(sizeof(log_file_output_stream));
   result->fn = log_stream_fn;
   result->memory = memory;
   result->logger = logger;
   result->module = module;
   result->tag = tag;
   return (cad_output_stream_t*)result;
}

/* ---------------------------------------------------------------- */

static void null_output_fn() {}
static cad_output_stream_t null_output = {
   (cad_output_stream_free_fn)null_output_fn,
   (cad_output_stream_put_fn)null_output_fn,
   (cad_output_stream_vput_fn)null_output_fn,
   (cad_output_stream_flush_fn)null_output_fn,
};

/* ---------------------------------------------------------------- */

typedef struct {
   circus_log_t fn;
   cad_memory_t memory;
   cad_hash_t *module_streams;
   log_level_t max_level;
   uv_logger_t *logger;
} circus_log_impl;

static cad_output_stream_t **__impl_set_log(circus_log_impl *this, const char *module, log_level_t max_level) {
   assert(module != NULL);
   assert(max_level < __LOG_MAX);

   log_level_t l;

   cad_output_stream_t **module_streams = this->module_streams->get(this->module_streams, module);
   if (module_streams == NULL) {
      module_streams = this->memory.malloc(__LOG_MAX * sizeof(cad_output_stream_t*));
      assert(module_streams != NULL);
      this->module_streams->set(this->module_streams, module, module_streams);
   } else {
      for (l = 0; l < __LOG_MAX; l++) {
         module_streams[l]->free(module_streams[l]);
      }
   }

   for (l = 0; l < __LOG_MAX; l++) {
      if (l <= this->max_level && l <= max_level) {
         module_streams[l] = new_log_stream(this->memory, this->logger, module, level_tag[l]);
      } else {
         module_streams[l] = &null_output;
      }
   }

   return module_streams;
}

static void impl_set_log(circus_log_impl *this, const char *module, log_level_t max_level) {
   __impl_set_log(this, module, max_level);
}

static int impl_is_log(circus_log_impl *this, const char *module, log_level_t level) {
   assert(module != NULL);
   assert(level < __LOG_MAX);

   cad_output_stream_t **module_streams = this->module_streams->get(this->module_streams, module);
   if (module_streams != NULL) {
      return module_streams[level] != &null_output;
   }
   return level <= this->max_level;
}

static void impl_set_format(circus_log_impl *this, const char *format) {
   assert(format != NULL);
   this->memory.free(this->logger->format);
   this->logger->format = this->memory.malloc(strlen(format) + 1);
   strcpy(this->logger->format, format);
}

static cad_output_stream_t *impl_stream(circus_log_impl *this, const char *module, log_level_t level) {
   assert(module != NULL);
   assert(level < __LOG_MAX);
   assert(this->logger != NULL);

   cad_output_stream_t **module_streams = this->module_streams->get(this->module_streams, module);
   if (module_streams == NULL) {
      module_streams = __impl_set_log(this, module, this->max_level);
   }

   return module_streams[level];
}

static void impl_module_stream_free_iterator(void *UNUSED(hash), int UNUSED(index), const char *UNUSED(key), cad_output_stream_t **module_streams, circus_log_impl *this) {
   int l;
   for (l = 0; l < __LOG_MAX; l++) {
      module_streams[l]->free(module_streams[l]);
   }
   this->memory.free(module_streams);
}

static void impl_close(circus_log_impl *this) {
   if (this->logger != NULL) {
      this->module_streams->clean(this->module_streams, (cad_hash_iterator_fn)impl_module_stream_free_iterator, this);
      this->module_streams->free(this->module_streams);
      this->logger->fn.free(this->logger);
      this->logger = NULL;
   }
}

static void impl_free(circus_log_impl *this) {
   impl_close(this);
   this->memory.free(this);
}

static circus_log_t impl_fn = {
   (circus_log_set_log_fn)impl_set_log,
   (circus_log_is_log_fn)impl_is_log,
   (circus_log_set_format_fn)impl_set_format,
   (circus_log_stream_fn)impl_stream,
   (circus_log_close_fn)impl_close,
   (circus_log_free_fn)impl_free,
};

circus_log_t *circus_new_log_file(cad_memory_t memory, const char *filename, log_level_t max_level) {
   assert(filename != NULL);
   assert(max_level < __LOG_MAX);

   circus_log_impl *result = memory.malloc(sizeof(circus_log_impl));
   if (result != NULL) {
      result->fn = impl_fn;
      result->memory = memory;
      result->module_streams = cad_new_hash(memory, cad_hash_strings);
      result->max_level = max_level;
      result->logger = new_logger_file(memory, filename);
   }
   return (circus_log_t*)result;
}

circus_log_t *circus_new_log_stdout(cad_memory_t memory, log_level_t max_level) {
   assert(max_level < __LOG_MAX);

   circus_log_impl *result = memory.malloc(sizeof(circus_log_impl));
   if (result != NULL) {
      result->fn = impl_fn;
      result->memory = memory;
      result->module_streams = cad_new_hash(memory, cad_hash_strings);
      result->max_level = max_level;
      result->logger = new_logger_stdout(memory);
   }
   return (circus_log_t*)result;
}

void log_error(circus_log_t *logger, const char *module, const char *format, ...) {
   va_list arg;
   cad_output_stream_t *s = logger->stream(logger, module, LOG_ERROR);
   va_start(arg, format);
   s->vput(s, format, arg);
   va_end(arg);
}

void log_warning(circus_log_t *logger, const char *module, const char *format, ...) {
   va_list arg;
   cad_output_stream_t *s = logger->stream(logger, module, LOG_WARNING);
   va_start(arg, format);
   s->vput(s, format, arg);
   va_end(arg);
}

void log_info(circus_log_t *logger, const char *module, const char *format, ...) {
   va_list arg;
   cad_output_stream_t *s = logger->stream(logger, module, LOG_INFO);
   va_start(arg, format);
   s->vput(s, format, arg);
   va_end(arg);
}

void log_debug(circus_log_t *logger, const char *module, const char *format, ...) {
   va_list arg;
   cad_output_stream_t *s = logger->stream(logger, module, LOG_DEBUG);
   va_start(arg, format);
   s->vput(s, format, arg);
   va_end(arg);
}
