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

#ifndef __CIRCUS_AUTOMATON_HANDLER_H
#define __CIRCUS_AUTOMATON_HANDLER_H

#include <cad_shared.h>

#include <circus_message.h>

typedef struct circus_automaton circus_automaton_t;

typedef enum {
   State_error = -1,
   State_read_from_client = 0,
   State_write_to_server,
   State_read_from_server,
   State_write_to_client,
} circus_automaton_state_e;

typedef circus_automaton_state_e (*circus_automaton_state_fn)(circus_automaton_t *this);
typedef circus_message_t *(*circus_automaton_message_fn)(circus_automaton_t *this);
typedef void (*circus_automaton_set_state_fn)(circus_automaton_t *this, circus_automaton_state_e state, circus_message_t *message);
typedef void (*circus_automaton_free_fn)(circus_automaton_t *this);

struct circus_automaton {
   circus_automaton_state_fn state;
   circus_automaton_message_fn message;
   circus_automaton_set_state_fn set_state;
   circus_automaton_free_fn free;
};

__PUBLIC__ circus_automaton_t *new_automaton(cad_memory_t memory);

#endif /* __CIRCUS_AUTOMATON_HANDLER_H */
