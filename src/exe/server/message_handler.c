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

#include <string.h>

#include <circus_vault.h>

#include "message_handler.h"

typedef struct {
   circus_server_message_handler_t fn;
   cad_memory_t memory;
   circus_vault_t *vault;
} impl_mh_t;

static void impl_mh_read(circus_channel_t *channel, impl_mh_t *this) {
   // TODO
   (void)channel; (void)this;
}

static void impl_mh_write(circus_channel_t *channel, impl_mh_t *this) {
   // TODO
   (void)channel; (void)this;
}

static void impl_register_to(impl_mh_t *this, circus_channel_t *channel) {
   channel->on_read(channel, (circus_channel_on_read_cb)impl_mh_read, this);
   channel->on_write(channel, (circus_channel_on_write_cb)impl_mh_write, this);
}

static void impl_free(impl_mh_t *this) {
   if (this->vault != NULL) {
      this->vault->free(this->vault);
   }
   this->memory.free(this);
}

static circus_server_message_handler_t impl_mh_fn = {
   (circus_server_message_handler_register_to_fn) impl_register_to,
   (circus_server_message_handler_free_fn) impl_free,
};

circus_server_message_handler_t *circus_message_handler(cad_memory_t memory, circus_config_t *config) {
   impl_mh_t *result;

   result = malloc(sizeof(impl_mh_t));
   assert(result != NULL);

   result->fn = impl_mh_fn;
   result->memory = memory;
   result->vault = circus_vault(memory, config);

   return (circus_server_message_handler_t*)result;
}
