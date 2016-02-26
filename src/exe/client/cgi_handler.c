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

#include <circus_log.h>

#include "cgi_handler.h"

typedef struct {
   circus_client_cgi_handler_t fn;
   cad_memory_t memory;
   circus_log_t *log;
} impl_cgi_t;

static void impl_cgi_read(circus_channel_t *channel, impl_cgi_t *this, cad_cgi_response_t *response) {
   // TODO
   (void)channel;(void)this;(void)response;
}

static void impl_cgi_write(circus_channel_t *channel, impl_cgi_t *this, cad_cgi_response_t *response) {
   // TODO
   (void)channel;(void)this;(void)response;
}

static void impl_register_to(impl_cgi_t *this, circus_channel_t *channel) {
   channel->on_read(channel, (circus_channel_on_read_cb)impl_cgi_read, this);
   channel->on_write(channel, (circus_channel_on_write_cb)impl_cgi_write, this);
}

static void impl_free(impl_cgi_t *this) {
   this->memory.free(this);
}

static circus_client_cgi_handler_t impl_cgi_fn = {
   (circus_client_cgi_handler_register_to_fn) impl_register_to,
   (circus_client_cgi_handler_free_fn) impl_free,
};

circus_client_cgi_handler_t *circus_cgi_handler(cad_memory_t memory, circus_log_t *log, circus_config_t *UNUSED(config)) {
   impl_cgi_t *result = memory.malloc(sizeof(impl_cgi_t));
   assert(result != NULL);

   result->fn = impl_cgi_fn;
   result->memory = memory;
   result->log = log;

   return I(result);
}
