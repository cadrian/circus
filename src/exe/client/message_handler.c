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

#include <errno.h>
#include <signal.h>
#include <string.h>

#include <circus_log.h>
#include <circus_message_impl.h>

#include "message_handler.h"

typedef struct {
   circus_client_message_handler_t fn;
   circus_message_visitor_reply_t vfn;
   cad_memory_t memory;
   circus_log_t *log;
   circus_channel_t *channel;
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

// circus_message_visitor_reply_logout_fn
static void visit_reply_logout(circus_message_visitor_reply_t *visitor, circus_message_reply_logout_t *visited) {
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
   (circus_message_visitor_reply_logout_fn)visit_reply_logout,
   (circus_message_visitor_reply_pass_fn)visit_reply_pass,
   (circus_message_visitor_reply_ping_fn)visit_reply_ping,
   (circus_message_visitor_reply_property_fn)visit_reply_property,
   (circus_message_visitor_reply_stop_fn)visit_reply_stop,
   (circus_message_visitor_reply_tags_fn)visit_reply_tags,
   (circus_message_visitor_reply_unset_fn)visit_reply_unset,
   (circus_message_visitor_reply_user_fn)visit_reply_user,
   (circus_message_visitor_reply_version_fn)visit_reply_version,
};

static void impl_mh_write(circus_channel_t *channel, impl_mh_t *this) {
   SET_CANARY();
   if (this->automaton->state(this->automaton) == State_write_to_server) {
      log_info(this->log, "message write");
      circus_message_t *msg = this->automaton->message(this->automaton);
      assert(msg != NULL);
      json_object_t *jmsg = msg->serialize(msg);
      assert(jmsg != NULL);
      char *szmsg = NULL;
      cad_output_stream_t *outmsg = new_cad_output_stream_from_string(&szmsg, this->memory);
      assert(outmsg != NULL);
      json_visitor_t *wmsg = json_write_to(outmsg, this->memory, json_compact);
      assert(wmsg != NULL);
      jmsg->accept(jmsg, wmsg);
      assert(outmsg != NULL);
      jmsg->accept(jmsg, json_kill());
      wmsg->free(wmsg);
      outmsg->free(outmsg);

      log_pii(this->log, ">> %s", szmsg);

      channel->write(channel, szmsg, strlen(szmsg));
      this->memory.free(szmsg);
      msg->free(msg);

      this->automaton->set_state(this->automaton, State_read_from_server, NULL);
   }
   CHECK_CANARY();
}

static void impl_mh_read(circus_channel_t *channel, impl_mh_t *this) {
   SET_CANARY();
   if (this->automaton->state(this->automaton) == State_read_from_server) {
      log_info(this->log, "message read");
      int buflen = 4096;
      int nbuf = 0;
      char *buf = this->memory.malloc(buflen);
      assert(buf != NULL);
      int n;
      do {
         n = channel->read(channel, buf + nbuf, buflen - nbuf);
         if (n > 0) {
            if (n + nbuf == buflen) {
               size_t bl = buflen * 2;
               buf = this->memory.realloc(buf, bl);
               buflen = bl;
            }
            nbuf += n;
         }
      } while (n > 0);

      log_pii(this->log, "<< %s", buf);

      cad_input_stream_t *inmsg = new_cad_input_stream_from_string(buf, this->memory);
      assert(inmsg != NULL);
      json_value_t *jmsg = json_parse(inmsg, NULL, NULL, this->memory);
      assert(jmsg != NULL);
      circus_message_t *msg = deserialize_circus_message(this->memory, (json_object_t*)jmsg); // TODO what if not an object?
      assert(msg != NULL);

      jmsg->accept(jmsg, json_kill());
      inmsg->free(inmsg);
      this->memory.free(buf);

      msg->accept(msg, (circus_message_visitor_t*)&(this->vfn));
      this->automaton->set_state(this->automaton, State_write_to_client, msg);
   }
   CHECK_CANARY();
}

static void on_write(circus_automaton_t *automaton, impl_mh_t *this) {
   assert(this->automaton == automaton);
   log_debug(this->log, "register on_write");
   this->channel->on_write(this->channel, (circus_channel_on_write_cb)impl_mh_write, NULL, this);
}

static void on_read(circus_automaton_t *automaton, impl_mh_t *this) {
   assert(this->automaton == automaton);
   log_debug(this->log, "register on_read");
   this->channel->on_read(this->channel, (circus_channel_on_read_cb)impl_mh_read, this);
}

static void impl_register_to(impl_mh_t *this, circus_channel_t *channel, circus_automaton_t *automaton) {
   assert(automaton != NULL);
   this->channel = channel;
   this->automaton = automaton;
   this->automaton->on_state(this->automaton, State_write_to_server, (circus_automaton_state_cb)on_write, this);
   this->automaton->on_state(this->automaton, State_read_from_server, (circus_automaton_state_cb)on_read, this);
}

static void impl_free(impl_mh_t *this) {
   this->memory.free(this);
}

static circus_client_message_handler_t impl_mh_fn = {
   (circus_client_message_handler_register_to_fn) impl_register_to,
   (circus_client_message_handler_free_fn) impl_free,
};

static void install_signals(impl_mh_t *mh) {
      struct sigaction action;
      action.sa_handler = SIG_IGN;
      sigemptyset(&action.sa_mask);
      action.sa_flags = 0;
      int n_pipe = sigaction(SIGPIPE, &action, NULL);
      if (n_pipe == -1) {
         log_warning(mh->log, "Could not install signal handler for SIGPIPE: %s", strerror(errno));
      }
}

circus_client_message_handler_t *circus_message_handler(cad_memory_t memory, circus_log_t *log, circus_config_t *UNUSED(config)) {
   impl_mh_t *result = memory.malloc(sizeof(impl_mh_t));
   assert(result != NULL);

   result->fn = impl_mh_fn;
   result->vfn = visitor_fn;
   result->memory = memory;
   result->log = log;
   result->channel = NULL;
   result->automaton = NULL;

   install_signals(result);

   return I(result);
}
