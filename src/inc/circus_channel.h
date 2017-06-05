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

#ifndef __CIRCUS_CHANNEL_H
#define __CIRCUS_CHANNEL_H

#include <cad_cgi.h>
#include <cad_shared.h>

#include <circus.h>
#include <circus_config.h>
#include <circus_log.h>

typedef struct circus_channel_s circus_channel_t;

/*
 * NOTE about the elliptic callbacks: for ZMQ there is no extra
 * argument; for CGI the response is given after the data.
 */

typedef void (*circus_channel_on_read_cb)(circus_channel_t *this, void *data, ...);
typedef void (*circus_channel_on_write_cb)(circus_channel_t *this, void *data, ...);
typedef void (*circus_channel_on_write_done_cb)(circus_channel_t *this, void *data);

typedef void (*circus_channel_on_read_fn)(circus_channel_t *this, circus_channel_on_read_cb cb, void *data);
typedef void (*circus_channel_on_write_fn)(circus_channel_t *this, circus_channel_on_write_cb cb, circus_channel_on_write_done_cb done_cb, void *data);
typedef int (*circus_channel_read_fn)(circus_channel_t *this, char *buffer, size_t buflen, ...);
typedef void (*circus_channel_write_fn)(circus_channel_t *this, const char *buffer, size_t buflen, ...);
typedef void (*circus_channel_free_fn)(circus_channel_t *this);

struct circus_channel_s {
   circus_channel_on_read_fn on_read;
   circus_channel_on_write_fn on_write;
   circus_channel_read_fn read;
   circus_channel_write_fn write;
   circus_channel_free_fn free;
};

__PUBLIC__ circus_channel_t *circus_zmq_server(cad_memory_t memory, circus_log_t *log, circus_config_t *config);
__PUBLIC__ circus_channel_t *circus_zmq_client(cad_memory_t memory, circus_log_t *log, circus_config_t *config);

__PUBLIC__ circus_channel_t *circus_cgi(cad_memory_t memory, circus_log_t *log, circus_config_t *config);

#endif /* __CIRCUS_CHANNEL_H */
