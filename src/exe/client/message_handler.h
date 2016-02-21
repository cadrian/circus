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

#ifndef __CIRCUS_CLIENT_MESSAGE_HANDLER_H
#define __CIRCUS_CLIENT_MESSAGE_HANDLER_H

#include <cad_shared.h>

#include <circus_channel.h>
#include <circus_config.h>
#include <circus_log.h>

typedef struct circus_client_message_handler_s circus_client_message_handler_t;

typedef void (*circus_client_message_handler_register_to_fn)(circus_client_message_handler_t *this, circus_channel_t *channel);
typedef void (*circus_client_message_handler_free_fn)(circus_client_message_handler_t *this);

struct circus_client_message_handler_s {
   circus_client_message_handler_register_to_fn register_to;
   circus_client_message_handler_free_fn free;
};

__PUBLIC__ circus_client_message_handler_t *circus_message_handler(cad_memory_t memory, circus_log_t *log, circus_config_t *config);

#endif /* __CIRCUS_CLIENT_MESSAGE_HANDLER_H */
