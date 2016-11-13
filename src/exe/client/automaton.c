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
   circus_log_t *log;
   circus_automaton_state_e state;
   circus_message_t *message;
   circus_automaton_state_cb cb[__STATE_MAX];
   void *cb_data[__STATE_MAX];
} automaton_impl;

static circus_automaton_state_e state(automaton_impl *this) {
   return this->state;
}

static circus_message_t *message(automaton_impl *this) {
   return this->message;
}

static void set_state(automaton_impl *this, circus_automaton_state_e state, circus_message_t *message) {
   assert(this->state != State_error && (state == State_error || state >= this->state));
   int changed = state != this->state;
   log_debug(this->log, "State: %d -> %d [%p]", this->state, state, message);
   this->state = state;
   this->message = message;
   if (changed && (state != State_error)) {
      circus_automaton_state_cb cb = this->cb[state];
      if (cb) {
         cb(I(this), this->cb_data[state]);
      }
   }
}

static void on_state(automaton_impl *this, circus_automaton_state_e state, circus_automaton_state_cb cb, void *data) {
   assert(state >= 0 && state < __STATE_MAX);
   this->cb[state] = cb;
   this->cb_data[state] = data;
   if ((this->state == state) && (cb != NULL)) {
      cb(I(this), data);
   }
}

static void free_automaton(automaton_impl *this) {
   log_debug(this->log, "Freeing automaton, state: %d", this->state);
   this->memory.free(this);
}

static circus_automaton_t fn = {
   (circus_automaton_state_fn) state,
   (circus_automaton_message_fn) message,
   (circus_automaton_set_state_fn) set_state,
   (circus_automaton_on_state_fn) on_state,
   (circus_automaton_free_fn) free_automaton,
};

circus_automaton_t *new_automaton(cad_memory_t memory, circus_log_t *log) {
   automaton_impl *result = memory.malloc(sizeof(automaton_impl));
   assert(result != NULL);
   result->fn = fn;
   result->memory = memory;
   result->log = log;
   result->state = State_started;
   result->message = NULL;
   int i;
   for (i = 0; i < __STATE_MAX; i++) {
      result->cb[i] = NULL;
      result->cb_data[i] = NULL;
   }
   return I(result);
}
