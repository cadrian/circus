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
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include <circus_channel.h>

typedef struct {
   circus_channel_t fn;
   cad_memory_t memory;
   circus_log_t *log;
   cad_cgi_t *cgi;
   circus_channel_on_read_cb read_cb;
   void *read_data;
   circus_channel_on_write_cb write_cb;
   void *write_data;
   uv_poll_t read_handle;
   uv_poll_t write_handle;
} cgi_impl_t;

static void impl_on_read(cgi_impl_t *this, circus_channel_on_read_cb cb, void *data) {
   this->read_cb = cb;
   this->read_data = data;
}

static void impl_on_write(cgi_impl_t *this, circus_channel_on_write_cb cb, void *data) {
   this->write_cb = cb;
   this->write_data = data;
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
   this->memory.free(this);
}

static circus_channel_t impl_fn = {
   (circus_channel_on_read_fn) impl_on_read,
   (circus_channel_on_write_fn) impl_on_write,
   (circus_channel_read_fn) impl_read,
   (circus_channel_write_fn) impl_write,
   (circus_channel_free_fn) impl_free,
};

static void impl_cgi_write_callback(uv_poll_t *handle, int status, int events) {
   cgi_impl_t *this = container_of(handle, cgi_impl_t, write_handle);
   int n;
   cad_cgi_response_t *response = handle->data;
   assert(response != NULL);
   if (status != 0) {
      log_warning(this->log, "channel_cgi", "impl_cgi_write_callback: status=%d", status);
      return;
   }
   log_debug(this->log, "channel_cgi", "impl_cgi_write_callback: event write: %s", events & UV_WRITABLE ? "true": "false");
   if (events & UV_WRITABLE) {
      if (this->write_cb != NULL) {
         log_debug(this->log, "channel_cgi", "impl_cgi_write_callback: calling callback");
         (this->write_cb)((circus_channel_t*)this, this->write_data, response);
         n = response->flush(response);
         assert(n == 0);
      }
      response->free(response);
      n = uv_poll_stop(&(this->write_handle));
      assert(n == 0);
   }
}

static void start_write(cgi_impl_t *this, cad_cgi_response_t *response) {
   int n;

   n = uv_poll_init(uv_default_loop(), &(this->write_handle), response->fd(response));
   assert(n == 0);
   this->write_handle.data = response;

   n = uv_poll_start(&(this->write_handle), UV_WRITABLE, impl_cgi_write_callback);
   assert(n == 0);
}

static void impl_cgi_read_callback(uv_poll_t *handle, int status, int events) {
   cgi_impl_t *this = handle->data;
   assert(this == container_of(handle, cgi_impl_t, read_handle));
   if (status != 0) {
      log_warning(this->log, "channel_cgi", "impl_cgi_read_callback: status=%d", status);
      return;
   }
   log_debug(this->log, "channel_cgi", "impl_cgi_read_callback: event read: %s", events & UV_READABLE ? "true": "false");
   if (events & UV_READABLE) {
      log_debug(this->log, "channel_cgi", "impl_cgi_write_callback: calling CGI run");
      cad_cgi_response_t *response = this->cgi->run(this->cgi);
      if (response != NULL) {
         start_write(this, response);
      } else {
         log_error(this->log, "channel_cgi", "NULL response!!");
      }
      int n = uv_poll_stop(&(this->read_handle));
      assert(n == 0);
   }
}

static void start_read(cgi_impl_t *this) {
   int n;

   n = uv_poll_init(uv_default_loop(), &(this->read_handle), this->cgi->fd(this->cgi));
   assert(n == 0);
   this->read_handle.data = this;

   n = uv_poll_start(&(this->read_handle), UV_READABLE, impl_cgi_read_callback);
   assert(n == 0);
}

static int cgi_handler(cad_cgi_t *cgi, cad_cgi_response_t *response, cgi_impl_t *this) {
   assert(this->cgi == cgi);
   log_debug(this->log, "channel_cgi", "cgi_handler: response=%p", response);
   if (this->read_cb != NULL) {
      (this->read_cb)((circus_channel_t*)this, this->read_data, response);
      return 0;
   }
   return 1;
}

circus_channel_t *circus_cgi(cad_memory_t memory, circus_log_t *log, circus_config_t *UNUSED(config)) {
   cgi_impl_t *result = memory.malloc(sizeof(cgi_impl_t));
   assert(result != NULL);

   cad_cgi_t *cgi = new_cad_cgi(memory, (cad_cgi_handle_cb)cgi_handler, result);
   assert(cgi != NULL);

   result->fn = impl_fn;
   result->memory = memory;
   result->log = log;
   result->cgi = cgi;
   result->read_cb = NULL;
   result->read_data = NULL;
   result->write_cb = NULL;
   result->write_data = NULL;
   result->read_handle.data = NULL;
   result->write_handle.data = NULL;

   start_read(result);

   return I(result);
}
