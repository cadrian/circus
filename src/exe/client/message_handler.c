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

#include <circus_log.h>
#include <circus_message_impl.h>

#include "message_handler.h"

typedef struct {
   circus_client_message_handler_t fn;
   circus_message_visitor_reply_t vfn;
   cad_memory_t memory;
   circus_log_t *log;
   circus_automaton_t *automaton;
} impl_mh_t;

// circus_message_visitor_reply_change_master_fn
static void visit_reply_change_master(circus_message_visitor_reply_t *visitor, circus_message_reply_change_master_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   // TODO
   (void)visited; (void)this;
}

// circus_message_visitor_reply_close_fn
static void visit_reply_close(circus_message_visitor_reply_t *visitor, circus_message_reply_close_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   // TODO
   (void)visited; (void)this;
}

// circus_message_visitor_reply_is_open_fn
static void visit_reply_is_open(circus_message_visitor_reply_t *visitor, circus_message_reply_is_open_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   // TODO
   (void)visited; (void)this;
}

// circus_message_visitor_reply_list_fn
static void visit_reply_list(circus_message_visitor_reply_t *visitor, circus_message_reply_list_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   // TODO
   (void)visited; (void)this;
}

// circus_message_visitor_reply_login_fn
static void visit_reply_login(circus_message_visitor_reply_t *visitor, circus_message_reply_login_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   // TODO
   (void)visited; (void)this;
}

// circus_message_visitor_reply_pass_fn
static void visit_reply_pass(circus_message_visitor_reply_t *visitor, circus_message_reply_pass_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   // TODO
   (void)visited; (void)this;
}

// circus_message_visitor_reply_ping_fn
static void visit_reply_ping(circus_message_visitor_reply_t *visitor, circus_message_reply_ping_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   // TODO
   (void)visited; (void)this;
}

// circus_message_visitor_reply_property_fn
static void visit_reply_property(circus_message_visitor_reply_t *visitor, circus_message_reply_property_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   // TODO
   (void)visited; (void)this;
}

// circus_message_visitor_reply_stop_fn
static void visit_reply_stop(circus_message_visitor_reply_t *visitor, circus_message_reply_stop_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   // TODO
   (void)visited; (void)this;
}

// circus_message_visitor_reply_tags_fn
static void visit_reply_tags(circus_message_visitor_reply_t *visitor, circus_message_reply_tags_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   // TODO
   (void)visited; (void)this;
}

// circus_message_visitor_reply_unset_fn
static void visit_reply_unset(circus_message_visitor_reply_t *visitor, circus_message_reply_unset_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   // TODO
   (void)visited; (void)this;
}

// circus_message_visitor_reply_user_fn
static void visit_reply_user(circus_message_visitor_reply_t *visitor, circus_message_reply_user_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   // TODO
   (void)visited; (void)this;
}

// circus_message_visitor_reply_version_fn
static void visit_reply_version(circus_message_visitor_reply_t *visitor, circus_message_reply_version_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   // TODO
   (void)visited; (void)this;
}

static circus_message_visitor_reply_t visitor_fn = {
   (circus_message_visitor_reply_change_master_fn)visit_reply_change_master,
   (circus_message_visitor_reply_close_fn)visit_reply_close,
   (circus_message_visitor_reply_is_open_fn)visit_reply_is_open,
   (circus_message_visitor_reply_list_fn)visit_reply_list,
   (circus_message_visitor_reply_login_fn)visit_reply_login,
   (circus_message_visitor_reply_pass_fn)visit_reply_pass,
   (circus_message_visitor_reply_ping_fn)visit_reply_ping,
   (circus_message_visitor_reply_property_fn)visit_reply_property,
   (circus_message_visitor_reply_stop_fn)visit_reply_stop,
   (circus_message_visitor_reply_tags_fn)visit_reply_tags,
   (circus_message_visitor_reply_unset_fn)visit_reply_unset,
   (circus_message_visitor_reply_user_fn)visit_reply_user,
   (circus_message_visitor_reply_version_fn)visit_reply_version,
};

static void impl_mh_read(circus_channel_t *channel, impl_mh_t *this) {
   if (this->automaton->state(this->automaton) == State_read_from_server) {
      // TODO
      (void)channel;
   }
}

static void impl_mh_write(circus_channel_t *channel, impl_mh_t *this) {
   if (this->automaton->state(this->automaton) == State_write_to_server) {
      // TODO
      (void)channel;
   }
}

static void impl_register_to(impl_mh_t *this, circus_channel_t *channel, circus_automaton_t *automaton) {
   assert(automaton != NULL);
   this->automaton = automaton;
   channel->on_read(channel, (circus_channel_on_read_cb)impl_mh_read, this);
   channel->on_write(channel, (circus_channel_on_write_cb)impl_mh_write, this);
}

static void impl_free(impl_mh_t *this) {
   this->memory.free(this);
}

static circus_client_message_handler_t impl_mh_fn = {
   (circus_client_message_handler_register_to_fn) impl_register_to,
   (circus_client_message_handler_free_fn) impl_free,
};

circus_client_message_handler_t *circus_message_handler(cad_memory_t memory, circus_log_t *log, circus_config_t *UNUSED(config)) {
   impl_mh_t *result = memory.malloc(sizeof(impl_mh_t));
   assert(result != NULL);

   result->fn = impl_mh_fn;
   result->vfn = visitor_fn;
   result->memory = memory;
   result->log = log;

   return I(result);
}
