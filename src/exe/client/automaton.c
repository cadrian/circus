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

#include "automaton.h"

typedef struct {
   circus_automaton_t fn;
   cad_memory_t memory;
   circus_automaton_state_e state;
   circus_message_t *message;
} automaton_impl;

static circus_automaton_state_e state(automaton_impl *this) {
   return this->state;
}

static circus_message_t *message(automaton_impl *this) {
   return this->message;
}

static void set_state(automaton_impl *this, circus_automaton_state_e state, circus_message_t *message) {
   assert(this->state != -1 && (state == -1 || state >= this->state));
   this->state = state;
   this->message = message;
}

static void free_automaton(automaton_impl *this) {
   this->memory.free(this);
}

static circus_automaton_t fn = {
   (circus_automaton_state_fn) state,
   (circus_automaton_message_fn) message,
   (circus_automaton_set_state_fn) set_state,
   (circus_automaton_free_fn) free_automaton,
};

circus_automaton_t *new_automaton(cad_memory_t memory) {
   automaton_impl *result = memory.malloc(sizeof(automaton_impl));
   assert(result != NULL);
   result->fn = fn;
   result->memory = memory;
   result->state = State_read_from_client;
   result->message = NULL;
   return I(result);
}
