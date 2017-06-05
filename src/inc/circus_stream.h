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

    Copyright © 2015-2017 Cyril Adrian <cyril.adrian@gmail.com>
*/

#ifndef __CIRCUS_STREAM_H
#define __CIRCUS_STREAM_H

#include <cad_shared.h>
#include <cad_stream.h>

#include <circus.h>

typedef struct circus_stream_s circus_stream_t;

/**
 * @return zero if need to stop reading
 */
typedef int (*circus_stream_read_fn)(circus_stream_t *stream, void *payload, const char *buffer, int len);
typedef void (*circus_stream_write_fn)(circus_stream_t *stream, void *payload);

typedef struct circus_stream_req_s circus_stream_req_t;
typedef void (*stream_req_free_fn)(circus_stream_req_t *this);
struct circus_stream_req_s {
   stream_req_free_fn free;
};

__PUBLIC__ circus_stream_req_t *circus_stream_req(cad_memory_t memory, const char *base, int len);

typedef void (*stream_free_fn)(circus_stream_t *stream);
typedef int (*stream_read_fn)(circus_stream_t *stream, circus_stream_req_t *req);
typedef int (*stream_write_fn)(circus_stream_t *stream, circus_stream_req_t *req);
typedef void (*stream_flush_fn)(circus_stream_t *stream);
struct circus_stream_s {
   stream_free_fn   free  ;
   stream_read_fn   read  ;
   stream_write_fn  write ;
   stream_flush_fn  flush ;
};

__PUBLIC__ circus_stream_t *new_stream_fd_read(cad_memory_t memory, int fd, circus_stream_read_fn on_read, void *payload);
__PUBLIC__ circus_stream_t *new_stream_fd_write(cad_memory_t memory, int fd, circus_stream_write_fn on_write, void *payload);

__PUBLIC__ circus_stream_t *new_stream_file_write(cad_memory_t memory, const char *filename, circus_stream_write_fn on_write, void *payload, int *uv_error);
__PUBLIC__ circus_stream_t *new_stream_file_append(cad_memory_t memory, const char *filename, circus_stream_write_fn on_write, void *payload, int *uv_error);
__PUBLIC__ circus_stream_t *new_stream_file_read(cad_memory_t memory, const char *filename, circus_stream_read_fn on_read, void *payload, int *uv_error);

#endif /* __CIRCUS_STREAM_H */
