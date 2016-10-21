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

#include <cad_cgi.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include <circus_channel.h>
#include <circus_stream.h>

#define DEFAULT_CGI_STREAM_CAPACITY 4096

typedef struct {
   char *data;
   size_t capacity;
   size_t index;
   size_t count;
} cgi_buffer;

static void init_buffer(cad_memory_t memory, cgi_buffer *buf) {
   buf->capacity = DEFAULT_CGI_STREAM_CAPACITY;
   buf->data = memory.malloc(buf->capacity);
   assert(buf->data != NULL);
   buf->index = 0;
   buf->count = 0;
}

static int buffer_vput(cad_memory_t memory, cgi_buffer *this, const char *format, va_list args) {
   int result;
   va_list copy;
   va_copy(copy, args);
   result = vsnprintf("", 0, format, copy);
   va_end(copy);
   if (result > 0) {
      size_t cap = this->capacity;
      while (cap < this->count + result) {
         cap *= 2;
      }
      if (cap > this->capacity) {
         char *buf = memory.malloc(cap);
         memcpy(buf, this->data, this->count);
         memory.free(this->data);
         this->data = buf;
         this->capacity = cap;
      }
      result = vsnprintf(this->data + this->count, this->capacity - this->count, format, args);
      this->count += result;
   }
   return result;
}

static int buffer_put(cad_memory_t memory, cgi_buffer *this, const char *format, ...) {
   int result;
   va_list args;
   va_start(args, format);
   result = buffer_vput(memory, this, format, args);
   va_end(args);
   return result;
}

typedef struct cgi_impl_s cgi_impl_t;

typedef struct {
   cad_input_stream_t fn;
   cad_memory_t memory;
   cgi_impl_t *cgi;
   cgi_buffer buffer;
} cgi_input_stream;

static void in_stream_free(cgi_input_stream *this) {
   this->memory.free(this->buffer.data);
   this->memory.free(this);
}

static int in_stream_next(cgi_input_stream *this) {
   if (this->buffer.index < this->buffer.count) {
      this->buffer.index++;
      return 0;
   }
   return -1;
}

static int in_stream_item(cgi_input_stream *this) {
   if (this->buffer.index <= this->buffer.count) {
      return this->buffer.data[this->buffer.index];
   }
   return -1;
}

static cad_input_stream_t cgi_in_fn = {
   (cad_input_stream_free_fn) in_stream_free,
   (cad_input_stream_next_fn) in_stream_next,
   (cad_input_stream_item_fn) in_stream_item,
};

static cgi_input_stream *new_cgi_in(cad_memory_t memory, cgi_impl_t *cgi) {
   cgi_input_stream *result = memory.malloc(sizeof(cgi_input_stream));
   assert(result != NULL);
   result->fn = cgi_in_fn;
   result->memory = memory;
   result->cgi = cgi;
   init_buffer(memory, &result->buffer);
   return result;
}

typedef struct {
   cad_output_stream_t fn;
   cad_memory_t memory;
   cgi_impl_t *cgi;
   cgi_buffer buffer;
} cgi_output_stream;

static void out_stream_free(cgi_output_stream *this) {
   this->memory.free(this->buffer.data);
   this->memory.free(this);
}

static int out_stream_vput(cgi_output_stream *this, const char *format, va_list args) {
   return buffer_vput(this->memory, &this->buffer, format, args);
}

static int out_stream_put(cgi_output_stream *this, const char *format, ...) {
   int result;
   va_list args;
   va_start(args, format);
   result = out_stream_vput(this, format, args);
   va_end(args);
   return result;
}

static int out_stream_flush(cgi_output_stream *UNUSED(this)) {
   return 0;
}

static cad_output_stream_t cgi_out_fn = {
   (cad_output_stream_free_fn) out_stream_free,
   (cad_output_stream_put_fn) out_stream_put,
   (cad_output_stream_vput_fn) out_stream_vput,
   (cad_output_stream_flush_fn) out_stream_flush,
};

static cgi_output_stream *new_cgi_out(cad_memory_t memory, cgi_impl_t *cgi) {
   cgi_output_stream *result = memory.malloc(sizeof(cgi_output_stream));
   assert(result != NULL);
   result->fn = cgi_out_fn;
   result->memory = memory;
   result->cgi = cgi;
   init_buffer(memory, &result->buffer);
   return result;
}

/* ---------------------------------------------------------------- */

struct cgi_impl_s {
   circus_channel_t fn;
   cad_memory_t memory;
   circus_log_t *log;
   cad_cgi_t *cgi;
   circus_channel_on_read_cb read_cb;
   void *read_data;
   circus_channel_on_write_cb write_cb;
   circus_channel_on_write_done_cb write_done_cb;
   void *write_data;
   circus_stream_t *cgi_in;
   circus_stream_t *cgi_out;
   cad_cgi_response_t *response;
   cgi_input_stream *cgi_in_stream;
   cgi_output_stream *cgi_out_stream;
};

static void start_read(cgi_impl_t *this) {
   log_debug(this->log, "Start read CGI");
   circus_stream_req_t *req = circus_stream_req(this->memory, NULL, 0);
   this->cgi_in->read(this->cgi_in, req);
   req->free(req);
   log_debug(this->log, "Started read CGI");
}

static void impl_on_read(cgi_impl_t *this, circus_channel_on_read_cb cb, void *data) {
   log_debug(this->log, "start reading CGI");
   this->read_cb = cb;
   this->read_data = data;
   start_read(this);
   log_debug(this->log, "started reading CGI");
}

static void start_write(cgi_impl_t *this) {
   log_debug(this->log, "Start write CGI");

   assert(this->response != NULL);
   if (this->write_cb == NULL) {
      log_debug(this->log, "no write callback (will write later)");
   } else {
      log_debug(this->log, "calling callback");
      (this->write_cb)((circus_channel_t*)this, this->write_data, this->response);
      int n = this->response->flush(this->response);
      if (n != 0) {
         log_warning(this->log, "error while flushing response: %s", strerror(errno));
      } else {
         this->response->free(this->response);
         this->response = NULL;

         log_debug(this->log, "Write CGI response");
         circus_stream_req_t *req = circus_stream_req(this->memory, this->cgi_out_stream->buffer.data, this->cgi_out_stream->buffer.count);
         this->cgi_out->write(this->cgi_out, req);
         this->cgi_out->flush(this->cgi_out);
         req->free(req);
      }
      if (this->write_done_cb != NULL) {
         log_debug(this->log, "CGI finished");
         (this->write_done_cb)((circus_channel_t*)this, this->write_data);
      }
   }
}

static void impl_on_write(cgi_impl_t *this, circus_channel_on_write_cb cb, circus_channel_on_write_done_cb done_cb, void *data) {
   log_debug(this->log, "start writing CGI");
   this->write_cb = cb;
   this->write_done_cb = done_cb;
   this->write_data = data;
   if (this->response == NULL) {
      log_debug(this->log, "no CGI response registered (will write later)");
   } else {
      start_write(this);
   }
}

static int impl_read(cgi_impl_t *this, char *buffer, size_t buflen, cad_cgi_response_t *UNUSED(response)) {
   int fd = this->cgi->fd(this->cgi);
   return read(fd, buffer, buflen);
}

static int impl_write(cgi_impl_t *UNUSED(this), const char *buffer, size_t buflen, cad_cgi_response_t *response) {
   cad_output_stream_t *stream = response->body(response);
   return stream->put(stream, "%*s", (int)(buflen > (size_t)INT_MAX ? INT_MAX : buflen), buffer);
}

static void impl_free(cgi_impl_t *this) {
   this->cgi->free(this->cgi);
   this->cgi_in->free(this->cgi_in);
   this->cgi_out->free(this->cgi_out);
   this->memory.free(this);
}

static circus_channel_t impl_fn = {
   (circus_channel_on_read_fn) impl_on_read,
   (circus_channel_on_write_fn) impl_on_write,
   (circus_channel_read_fn) impl_read,
   (circus_channel_write_fn) impl_write,
   (circus_channel_free_fn) impl_free,
};

static int cgi_on_read(circus_stream_t *this, cgi_impl_t *cgi, const char *buffer, int len) {
   int result = 0;
   assert(cgi->cgi_in == this);
   log_debug(cgi->log, "Reading CGI (%p, %d)", buffer, len);
   cgi_input_stream *in = cgi->cgi_in_stream;
   if (len >= 0) {
      buffer_put(in->memory, &in->buffer, "%*s", len, buffer);
      result = 1;
   } else {
      assert(len == -1 /* EOF */);
      log_debug(cgi->log, "calling CGI run");
      cad_cgi_response_t *response = cgi->cgi->run(cgi->cgi);
      log_debug(cgi->log, "returned from CGI run, response=%p", response);
      if (response != NULL) {
         cgi->response = response;
         log_debug(cgi->log, "got response from CGI run");
         start_write(cgi);
      } else {
         log_error(cgi->log, "NULL response!!");
      }
   }
   return result;
}

static void cgi_on_write(circus_stream_t *this, cgi_impl_t *cgi) {
   assert(cgi->cgi_out == this);
   //assert(cgi->response != NULL);
   log_debug(cgi->log, "response written.");
}

static int cgi_handler(cad_cgi_t *cgi, cad_cgi_response_t *response, cgi_impl_t *this) {
   assert(this->cgi == cgi);
   log_debug(this->log, "response=%p", response);
   if (this->read_cb != NULL) {
      (this->read_cb)((circus_channel_t*)this, this->read_data, response);
      return 0;
   } else {
      log_warning(this->log, "no read callback!");
   }
   return 1;
}

circus_channel_t *circus_cgi(cad_memory_t memory, circus_log_t *log, circus_config_t *UNUSED(config)) {
   cgi_impl_t *result = memory.malloc(sizeof(cgi_impl_t));
   assert(result != NULL);

   result->fn = impl_fn;
   result->memory = memory;
   result->log = log;
   result->read_cb = NULL;
   result->read_data = NULL;
   result->write_cb = NULL;
   result->write_data = NULL;

   cgi_input_stream *cgi_in = new_cgi_in(memory, result);
   assert(cgi_in != NULL);
   result->cgi_in_stream = cgi_in;

   cgi_output_stream *cgi_out = new_cgi_out(memory, result);
   assert(cgi_out != NULL);
   result->cgi_out_stream = cgi_out;

   cad_cgi_t *cgi = new_cad_cgi_stream(memory, (cad_cgi_handle_cb)cgi_handler, result, I(cgi_in), I(cgi_out));
   assert(cgi != NULL);
   result->cgi = cgi;

   result->cgi_in = new_stream_fd_read(memory, STDIN_FILENO, (circus_stream_read_fn)cgi_on_read, result);
   if (result->cgi_in == NULL) {
      log_error(log, "CGI init failed for read");
   }

   result->cgi_out = new_stream_fd_write(memory, STDOUT_FILENO, (circus_stream_write_fn)cgi_on_write, result);
   if (result->cgi_out == NULL) {
      log_error(log, "CGI init failed for write");
   }

   return I(result);
}
