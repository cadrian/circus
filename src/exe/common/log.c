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

#include <string.h>
#include <sys/time.h>

#include <cad_shared.h>
#include <cad_hash.h>

#include <uv.h>

#include <circus_log.h>
#include <circus_stream.h>
#include <circus_time.h>

static const char* level_tag[] = {"ERROR", "WARNING", "INFO", "DEBUG", "PII"};

static int format_log(char *buf, const int buflen, const char *format, const struct timeval *tv, const char *tag, const char *module, const char *message) {
   int result = 0;
   int state = 0;
   struct tm *tm = gmtime(&(tv->tv_sec));
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

typedef struct {
   cad_output_stream_t fn;
   cad_memory_t memory;
   circus_stream_t *stream;
   char **format;
   const char *module;
   const char *tag;
} log_file_output_stream;

static void log_free_stream(log_file_output_stream *this) {
   this->memory.free(this);
}

static void log_vput_stream(log_file_output_stream *this, const char *format, va_list args) {
   SET_CANARY();

   circus_stream_req_t *req;
   struct timeval tv = now();
   char *message = NULL;
   int n;
   char *logline = NULL;

   message = vszprintf(this->memory, NULL, format, args);
   assert(message != NULL);

   n = format_log("", 0, *(this->format), &tv, this->tag, this->module, message);
   logline = this->memory.malloc(n + 1);
   assert(logline != NULL);
   format_log(logline, n + 1, *(this->format), &tv, this->tag, this->module, message);
   this->memory.free(message);

   req = circus_stream_req(this->memory, logline, n);
   this->memory.free(logline);

   this->stream->write(this->stream, req);

   CHECK_CANARY();
}

static void log_put_stream(log_file_output_stream *this, const char *format, ...) {
   va_list args;
   va_start(args, format);
   log_vput_stream(this, format, args);
   va_end(args);
}

static void log_flush_stream(log_file_output_stream *this) {
   this->stream->flush(this->stream);
}

cad_output_stream_t log_stream_fn = {
   .free  = (cad_output_stream_free_fn)log_free_stream,
   .put   = (cad_output_stream_put_fn)log_put_stream,
   .vput  = (cad_output_stream_vput_fn)log_vput_stream,
   .flush = (cad_output_stream_flush_fn)log_flush_stream,
};

static cad_output_stream_t *new_log_stream(cad_memory_t memory, circus_stream_t *stream, const char *module, const char *tag, char **format) {
   log_file_output_stream *result = memory.malloc(sizeof(log_file_output_stream));
   result->fn = log_stream_fn;
   result->memory = memory;
   result->stream = stream;
   result->format = format;
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
   circus_stream_t *stream;
   char *format;
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
         module_streams[l] = new_log_stream(this->memory, this->stream, module, level_tag[l], &(this->format));
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
   this->memory.free(this->format);
   this->format = szprintf(this->memory, NULL, "%s", format);
}

static cad_output_stream_t *impl_stream(circus_log_impl *this, const char *module, log_level_t level) {
   assert(module != NULL);
   assert(level >= LOG_ERROR && level < __LOG_MAX);
   assert(this->stream != NULL);

   cad_output_stream_t **module_streams = this->module_streams->get(this->module_streams, module);
   if (module_streams == NULL) {
      module_streams = __impl_set_log(this, module, this->max_level);
   }

   return module_streams[level];
}

static void impl_module_stream_free_iterator(void *UNUSED(hash), int UNUSED(index), const char *UNUSED(key), cad_output_stream_t **module_streams, circus_log_impl *this) {
   int l;
   for (l = 0; l < __LOG_MAX; l++) {
      module_streams[l]->flush(module_streams[l]);
      module_streams[l]->free(module_streams[l]);
   }
   this->memory.free(module_streams);
}

static void impl_close(circus_log_impl *this) {
   if (this->stream != NULL) {
      this->module_streams->clean(this->module_streams, (cad_hash_iterator_fn)impl_module_stream_free_iterator, this);
      this->module_streams->free(this->module_streams);
      this->stream->free(this->stream);
      this->stream = NULL;
   }
}

static void impl_free(circus_log_impl *this) {
   // running the default loop just ensures that all the log messages are flushed and freed.
   // uv_run(uv_default_loop(), UV_RUN_DEFAULT);
   // impl_close(this);
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
      int e = 0;
      result->fn = impl_fn;
      result->memory = memory;
      result->module_streams = cad_new_hash(memory, cad_hash_strings);
      result->max_level = max_level;
      result->stream = new_stream_file_append(memory, filename, NULL, NULL, &e);
      if (e < 0) {
         fprintf(stderr, "Error opening log file (%s): %s\n", filename, uv_strerror(e));
         memory.free(result);
         result = NULL;
      } else {
         result->format = szprintf(memory, NULL, "%s", DEFAULT_FORMAT);
      }
   }
   return I(result);
}

circus_log_t *circus_new_log_file_descriptor(cad_memory_t memory, log_level_t max_level, int fd) {
   assert(max_level < __LOG_MAX);

   circus_log_impl *result = memory.malloc(sizeof(circus_log_impl));
   if (result != NULL) {
      result->fn = impl_fn;
      result->memory = memory;
      result->module_streams = cad_new_hash(memory, cad_hash_strings);
      result->max_level = max_level;
      result->stream = new_stream_fd_write(memory, fd, NULL, NULL);
      result->format = szprintf(memory, NULL, "%s", DEFAULT_FORMAT);
   }
   return I(result);
}

void logger_error(circus_log_t *logger, const char *module, int line, const char *format, ...) {
   va_list arg;
   cad_output_stream_t *s = logger->stream(logger, module, LOG_ERROR);
   int n = snprintf("", 0, "%d: %s", line, format) + 1;
   char *fmt = alloca(n);
   snprintf(fmt, n, "%d: %s", line, format);
   va_start(arg, format);
   s->vput(s, fmt, arg);
   va_end(arg);
}

void logger_warning(circus_log_t *logger, const char *module, int line, const char *format, ...) {
   va_list arg;
   cad_output_stream_t *s = logger->stream(logger, module, LOG_WARNING);
   int n = snprintf("", 0, "%d: %s", line, format) + 1;
   char *fmt = alloca(n);
   snprintf(fmt, n, "%d: %s", line, format);
   va_start(arg, format);
   s->vput(s, fmt, arg);
   va_end(arg);
}

void logger_info(circus_log_t *logger, const char *module, int line, const char *format, ...) {
   va_list arg;
   cad_output_stream_t *s = logger->stream(logger, module, LOG_INFO);
   int n = snprintf("", 0, "%d: %s", line, format) + 1;
   char *fmt = alloca(n);
   snprintf(fmt, n, "%d: %s", line, format);
   va_start(arg, format);
   s->vput(s, fmt, arg);
   va_end(arg);
}

void logger_debug(circus_log_t *logger, const char *module, int line, const char *format, ...) {
   va_list arg;
   cad_output_stream_t *s = logger->stream(logger, module, LOG_DEBUG);
   int n = snprintf("", 0, "%d: %s", line, format) + 1;
   char *fmt = alloca(n);
   snprintf(fmt, n, "%d: %s", line, format);
   va_start(arg, format);
   s->vput(s, fmt, arg);
   va_end(arg);
}

void logger_pii(circus_log_t *logger, const char *module, int line, const char *format, ...) {
   va_list arg;
   cad_output_stream_t *s = logger->stream(logger, module, LOG_PII);
   int n = snprintf("", 0, "%d: %s", line, format) + 1;
   char *fmt = alloca(n);
   snprintf(fmt, n, "%d: %s", line, format);
   va_start(arg, format);
   s->vput(s, fmt, arg);
   va_end(arg);
}
