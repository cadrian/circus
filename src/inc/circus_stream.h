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

#ifndef __CIRCUS_STREAM_H
#define __CIRCUS_STREAM_H

#include <cad_shared.h>

#include <circus.h>

typedef struct write_req_s write_req_t;
__PUBLIC__ write_req_t *circus_write_req(cad_memory_t memory, const char *base, int len);

typedef struct read_req_s read_req_t;
__PUBLIC__ read_req_t *circus_read_req(cad_memory_t memory, char *base, int len);

typedef struct circus_stream_s circus_stream_t;
typedef void (*stream_free_fn)(circus_stream_t *stream);
typedef int (*stream_read_fn)(circus_stream_t *stream, read_req_t *req);
typedef int (*stream_write_fn)(circus_stream_t *stream, write_req_t *req);
typedef void (*stream_flush_fn)(circus_stream_t *stream);
struct circus_stream_s {
   stream_free_fn  free ;
   stream_read_fn  read ;
   stream_write_fn write;
   stream_flush_fn flush;
};

__PUBLIC__ circus_stream_t *new_stream_fd(cad_memory_t memory, int fd);
__PUBLIC__ circus_stream_t *new_stream_file_write(cad_memory_t memory, const char *filename);
__PUBLIC__ circus_stream_t *new_stream_file_append(cad_memory_t memory, const char *filename);
__PUBLIC__ circus_stream_t *new_stream_file_read(cad_memory_t memory, const char *filename);

#endif /* __CIRCUS_STREAM_H */
