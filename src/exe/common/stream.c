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

#include <cad_shared.h>

#include <uv.h>

#include <circus_stream.h>

typedef struct cb_s {
   union {
      circus_stream_read_fn read;
      circus_stream_write_fn write;
   } on;
   void *payload;
} cb_t;

struct circus_stream_req_s {
   union {
      uv_stream_t *read;
      uv_write_t write;
      uv_fs_t fs;
   } req;
   cad_memory_t memory;
   circus_stream_t *stream;
   uv_buf_t buf;
   void (*cleanup)(circus_stream_req_t*);
   cb_t cb;
};

static void cleanup_stream(circus_stream_req_t *req) {
   req->memory.free(req->buf.base);
   req->buf = uv_buf_init(NULL, 0);
}

static void cleanup_fs(circus_stream_req_t *req) {
   req->memory.free(req->buf.base);
   req->buf = uv_buf_init(NULL, 0);
   uv_fs_req_cleanup(&req->req.fs);
}

static void on_write(circus_stream_req_t* req, int UNUSED(status)) {
   if (req->cb.on.write != NULL) {
      req->cb.on.write(req->stream, req->cb.payload);
   }
   req->cleanup(req);
   req->memory.free(req);
}

static void on_read_fs(circus_stream_req_t* req, int UNUSED(status)) {
   int more = 0;
   if (req->cb.on.read != NULL) {
      more = req->cb.on.read(req->stream, req->cb.payload, req->buf.base, req->buf.len);
   }
   if (!more) {
      req->cleanup(req);
      req->memory.free(req);
   }
}

static void on_read_stream(circus_stream_req_t* req, ssize_t nread, const uv_buf_t *buf) {
   assert(buf == &(req->buf));
   int more = 0;
   if (nread >= 0 && req->cb.on.read != NULL) {
      more = req->cb.on.read(req->stream, req->cb.payload, req->buf.base, (int)nread);
   }
   if (!more) {
      int n = uv_read_stop(req->req.read);
      assert(n == 0);
      req->cleanup(req);
      req->memory.free(req);
   }
}

static void stream_read_alloc(circus_stream_req_t *this, size_t suggested_size, uv_buf_t *buf) {
   if (suggested_size > this->buf.len) {
      this->memory.free(this->buf.base);
      this->buf = uv_buf_init(this->memory.malloc(suggested_size), suggested_size);
   }
   *buf = this->buf;
}

circus_stream_req_t *circus_stream_req(cad_memory_t memory, const char *base, int len) {
   circus_stream_req_t *result = memory.malloc(sizeof(circus_stream_req_t));
   int base_len = len;
   assert(result != NULL);

   result->memory = memory;
   if (base != NULL && len > 0) {
      char *buf_base = szprintf(memory, &base_len, "%*s", len, base);
      result->buf = uv_buf_init(buf_base, base_len);
   } else {
      result->buf = uv_buf_init(NULL, 0);
   }
   result->cleanup = NULL;

   return result;
}

/* ---------------------------------------------------------------- */

typedef struct stream_impl_s {
   circus_stream_t fn;
   cad_memory_t memory;
   uv_stream_t *stream;
   uv_file fd;
   int64_t offset;
   cb_t cb;
} stream_impl_t;

static void file_free(stream_impl_t *this) {
   assert(this->stream == NULL);
   this->memory.free(this);
}

static void file_read(stream_impl_t *this, circus_stream_req_t *req) {
   assert(req->cleanup == NULL);
   int64_t offset = this->offset;
   req->cleanup = cleanup_fs;
   req->cb = this->cb;
   uv_fs_read(uv_default_loop(), &req->req.fs, this->fd, &req->buf, 1, offset, (uv_fs_cb)on_read_fs);
}

static void file_write(stream_impl_t *this, circus_stream_req_t *req) {
   assert(req->cleanup == NULL);
   int64_t offset = this->offset;
   req->cleanup = cleanup_fs;
   this->offset += req->buf.len;
   req->cb = this->cb;
   uv_fs_write(uv_default_loop(), &req->req.fs, this->fd, &req->buf, 1, offset, (uv_fs_cb)on_write);
}

static void file_flush(stream_impl_t *this) {
   uv_fs_t req;
   uv_fs_fdatasync(uv_default_loop(), &req, this->fd, NULL);
   // TODO: be sure to wait that all the data has been written before
   // flushing. This will need some clever use of this->offset
   // (expected offset) and a new counter in the on_write callback
   // (actual written offset) + using a uv_mutex_t and uv_cond_t
   // barrier
}

/**
 * FILE is used when stderr was redirected
 */
static circus_stream_t stream_file_fn = {
   (stream_free_fn)file_free,
   (stream_read_fn)file_read,
   (stream_write_fn)file_write,
   (stream_flush_fn)file_flush,
};

/* ---------------------------------------------------------------- */

static void tty_free(stream_impl_t *this) {
   uv_tty_reset_mode();
   this->memory.free(this->stream);
   this->memory.free(this);
}

static void tty_read(stream_impl_t *this, circus_stream_req_t *req) {
   assert(req->cleanup == NULL);
   req->req.read = this->stream;
   assert(this->stream == (uv_stream_t*)req);
   req->cleanup = cleanup_fs;
   req->cb = this->cb;
   uv_read_start(this->stream, (uv_alloc_cb)stream_read_alloc, (uv_read_cb)on_read_stream);
}

static void tty_write(stream_impl_t *this, circus_stream_req_t *req) {
   assert(req->cleanup == NULL);
   req->cleanup = cleanup_stream;
   req->cb = this->cb;
   uv_write(&req->req.write, this->stream, &req->buf, 1, (uv_write_cb)on_write);
}

static void tty_flush(stream_impl_t *UNUSED(this)) {
   // TODO: just some kind of barrier too (see log_flush_file for explanations)
}

/**
 * TTY is used for normal tty-attached stderr
 */
static circus_stream_t stream_tty_fn = {
   (stream_free_fn)tty_free,
   (stream_read_fn)tty_read,
   (stream_write_fn)tty_write,
   (stream_flush_fn)tty_flush,
};

/* ---------------------------------------------------------------- */

static void pipe_free(stream_impl_t *this) {
   uv_fs_t req;
   uv_fs_close(uv_default_loop(), &req, this->fd, NULL);
   this->memory.free(this->stream);
   this->memory.free(this);
}

static void pipe_read(stream_impl_t *this, circus_stream_req_t *req) {
   assert(req->cleanup == NULL);
   req->req.read = this->stream;
   assert(this->stream == (uv_stream_t*)req);
   req->cleanup = cleanup_fs;
   req->cb = this->cb;
   uv_read_start(this->stream, (uv_alloc_cb)stream_read_alloc, (uv_read_cb)on_read_stream);
}

static void pipe_write(stream_impl_t *this, circus_stream_req_t *req) {
   assert(req->cleanup == NULL);
   req->cleanup = cleanup_stream;
   req->cb = this->cb;
   uv_write(&req->req.write, this->stream, &req->buf, 1, (uv_write_cb)on_write);
}

static void pipe_flush(stream_impl_t *UNUSED(this)) {
   // TODO: just some kind of barrier too (see log_flush_file for explanations)
}

/**
 * PIPE is used for logs written to file (not for stderr, see FILE)
 */
static circus_stream_t stream_pipe_fn = {
   (stream_free_fn)pipe_free,
   (stream_read_fn)pipe_read,
   (stream_write_fn)pipe_write,
   (stream_flush_fn)pipe_flush,
};

/* ---------------------------------------------------------------- */

static stream_impl_t *new_stream_file(cad_memory_t memory, const char *filename, int flags, int *uv_error) {
   assert(filename != NULL);

   stream_impl_t *result = NULL;
   uv_fs_t req;
   uv_file fd = uv_fs_open(uv_default_loop(), &req, filename, flags, 0600, NULL);
   if (fd >= 0) {
      result = memory.malloc(sizeof(stream_impl_t));
      uv_pipe_t *pipe = memory.malloc(sizeof(uv_pipe_t));
      uv_pipe_init(uv_default_loop(), pipe, 0);
      uv_pipe_open(pipe, fd);

      result->fn = stream_pipe_fn;
      result->memory = memory;
      result->stream = (uv_stream_t*)pipe;
      result->fd = fd;
      result->offset = 0;
   } else if (uv_error != NULL) {
      *uv_error = fd;
   }

   return result;
}

circus_stream_t *new_stream_file_write(cad_memory_t memory, const char *filename, circus_stream_write_fn on_write_cb, void *payload, int *uv_error) {
   stream_impl_t *result = new_stream_file(memory, filename, O_WRONLY | O_CREAT, uv_error);
   if (result != NULL) {
      result->cb.on.write = on_write_cb;
      result->cb.payload = payload;
   }
   return I(result);
}

circus_stream_t *new_stream_file_append(cad_memory_t memory, const char *filename, circus_stream_write_fn on_write_cb, void *payload, int *uv_error) {
   stream_impl_t *result = new_stream_file(memory, filename, O_WRONLY | O_CREAT | O_APPEND, uv_error);
   if (result != NULL) {
      result->cb.on.write = on_write_cb;
      result->cb.payload = payload;
   }
   return I(result);
}

circus_stream_t *new_stream_file_read(cad_memory_t memory, const char *filename, circus_stream_read_fn on_read_cb, void *payload, int *uv_error) {
   stream_impl_t *result = new_stream_file(memory, filename, O_RDONLY, uv_error);
   if (result != NULL) {
      result->cb.on.read = on_read_cb;
      result->cb.payload = payload;
   }
   return I(result);
}

static stream_impl_t *new_stream_fd(cad_memory_t memory, int fd) {
   stream_impl_t *result = memory.malloc(sizeof(stream_impl_t));
   assert(result != NULL);
   uv_tty_t *tty;
   uv_pipe_t *pipe;
   uv_handle_type handle_type = uv_guess_handle(fd);
   result->memory = memory;
   result->fd = fd;
   result->offset = 0;
   switch(handle_type) {
   case UV_TTY:
      tty = memory.malloc(sizeof(uv_tty_t));
      uv_tty_init(uv_default_loop(), tty, 1, 0);
      uv_tty_set_mode(tty, UV_TTY_MODE_NORMAL);
      result->fn = stream_tty_fn;
      result->stream = (uv_stream_t*)tty;
      break;
   case UV_NAMED_PIPE:
      pipe = memory.malloc(sizeof(uv_pipe_t));
      uv_pipe_init(uv_default_loop(), pipe, 0);
      uv_pipe_open(pipe, 1);
      result->fn = stream_pipe_fn;
      result->stream = (uv_stream_t*)pipe;
      break;
   case UV_FILE:
      result->fn = stream_file_fn;
      result->stream = NULL;
      break;
   default:
      fprintf(stderr, "handle_type=%d not handled...\n", handle_type);
      crash();
   }
   return result;
}

circus_stream_t *new_stream_fd_read(cad_memory_t memory, int fd, circus_stream_read_fn on_read_cb, void *payload) {
   stream_impl_t *result = new_stream_fd(memory, fd);
   result->cb.on.read = on_read_cb;
   result->cb.payload = payload;
   return I(result);
}

circus_stream_t *new_stream_fd_write(cad_memory_t memory, int fd, circus_stream_write_fn on_write_cb, void *payload) {
   stream_impl_t *result = new_stream_fd(memory, fd);
   result->cb.on.write = on_write_cb;
   result->cb.payload = payload;
   return I(result);
}
