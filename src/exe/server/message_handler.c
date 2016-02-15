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

#include <cad_stream.h>
#include <json.h>
#include <string.h>
#include <uv.h>

#include <circus_log.h>
#include <circus_message_impl.h>
#include <circus_session.h>
#include <circus_vault.h>

#include "message_handler.h"

typedef struct {
   circus_server_message_handler_t fn;
   circus_message_visitor_query_t vfn;
   cad_memory_t memory;
   circus_log_t *log;
   circus_vault_t *vault;
   circus_session_t *session;
   int running;
   circus_message_t *reply;
} impl_mh_t;

static void visit_query_change_master(circus_message_visitor_query_t *visitor, circus_message_query_change_master_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   // TODO
   (void)visited; (void)this;
}

static void visit_query_close(circus_message_visitor_query_t *visitor, circus_message_query_close_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   // TODO
   (void)visited; (void)this;
}

static void visit_query_is_open(circus_message_visitor_query_t *visitor, circus_message_query_is_open_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   // TODO
   (void)visited; (void)this;
}

static void visit_query_all_list(circus_message_visitor_query_t *visitor, circus_message_query_all_list_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   // TODO
   (void)visited; (void)this;
}

static void visit_query_tag_list(circus_message_visitor_query_t *visitor, circus_message_query_tag_list_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   // TODO
   (void)visited; (void)this;
}

static void visit_query_login(circus_message_visitor_query_t *visitor, circus_message_query_login_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   const char *userid = visited->userid(visited);
   const char *password = visited->password(visited);
   log_info(this->log, "message_handler", "Login: user %s", userid);
   circus_user_t *user = this->vault->get(this->vault, userid, password);
   circus_message_reply_login_t *reply = NULL;
   if (user == NULL) {
      reply = new_circus_message_reply_login(this->memory, "Invalid credentials", "", "");
   } else {
      circus_session_data_t *data = this->session->set(this->session, user);
      reply = new_circus_message_reply_login(this->memory, "", data->sessionid(data), data->token(data));
   }
   this->reply = I(reply);
}

static void visit_query_get_pass(circus_message_visitor_query_t *visitor, circus_message_query_get_pass_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   // TODO
   (void)visited; (void)this;
}

static void visit_query_set_pass_pass(circus_message_visitor_query_t *visitor, circus_message_query_set_pass_pass_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   // TODO
   (void)visited; (void)this;
}

static void visit_query_set_recipe_pass(circus_message_visitor_query_t *visitor, circus_message_query_set_recipe_pass_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   // TODO
   (void)visited; (void)this;
}

static void visit_query_ping(circus_message_visitor_query_t *visitor, circus_message_query_ping_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   const char *phrase = visited->phrase(visited);
   log_info(this->log, "message_handler", "Ping: %s", phrase);
   circus_message_reply_ping_t *ping = new_circus_message_reply_ping(this->memory, "", phrase);
   this->reply = I(ping);
}

static void visit_query_set_property(circus_message_visitor_query_t *visitor, circus_message_query_set_property_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   // TODO
   (void)visited; (void)this;
}

static void visit_query_unset_property(circus_message_visitor_query_t *visitor, circus_message_query_unset_property_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   // TODO
   (void)visited; (void)this;
}

static void visit_query_stop(circus_message_visitor_query_t *visitor, circus_message_query_stop_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   const char *reason = visited->reason(visited);
   const char *sessionid = visited->sessionid(visited);
   const char *token = visited->token(visited);
   int ok = 0;

   circus_session_data_t *data = this->session->get(this->session, sessionid, token);
   if (data == NULL) {
      log_error(this->log, "message_handler", "Stop query REFUSED, unknown session or invalid token");
      token = "";
   } else {
      circus_user_t *user = data->user(data);
      if (!user->is_admin(user)) {
         log_error(this->log, "message_handler", "Stop query REFUSED, user %s not admin", user->name(user));
      } else {
         log_warning(this->log, "message_handler", "Stopping: %s", reason);
         this->running = 0;
         uv_stop(uv_default_loop());
         ok = 1;
      }
      token = data->set_token(data);
   }

   circus_message_reply_stop_t *stop = new_circus_message_reply_stop(this->memory, ok ? "" : "refused", token);
   this->reply = I(stop);
}

static void visit_query_tags(circus_message_visitor_query_t *visitor, circus_message_query_tags_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   // TODO
   (void)visited; (void)this;
}

static void visit_query_unset(circus_message_visitor_query_t *visitor, circus_message_query_unset_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   // TODO
   (void)visited; (void)this;
}

static void visit_query_version(circus_message_visitor_query_t *visitor, circus_message_query_version_t *visited) {
   impl_mh_t *this = container_of(visitor, impl_mh_t, vfn);
   // TODO
   (void)visited; (void)this;
}

static circus_message_visitor_query_t visitor_fn = {
   (circus_message_visitor_query_change_master_fn)visit_query_change_master,
   (circus_message_visitor_query_close_fn)visit_query_close,
   (circus_message_visitor_query_is_open_fn)visit_query_is_open,
   (circus_message_visitor_query_all_list_fn)visit_query_all_list,
   (circus_message_visitor_query_tag_list_fn)visit_query_tag_list,
   (circus_message_visitor_query_login_fn)visit_query_login,
   (circus_message_visitor_query_get_pass_fn)visit_query_get_pass,
   (circus_message_visitor_query_set_pass_pass_fn)visit_query_set_pass_pass,
   (circus_message_visitor_query_set_recipe_pass_fn)visit_query_set_recipe_pass,
   (circus_message_visitor_query_ping_fn)visit_query_ping,
   (circus_message_visitor_query_set_property_fn)visit_query_set_property,
   (circus_message_visitor_query_unset_property_fn)visit_query_unset_property,
   (circus_message_visitor_query_stop_fn)visit_query_stop,
   (circus_message_visitor_query_tags_fn)visit_query_tags,
   (circus_message_visitor_query_unset_fn)visit_query_unset,
   (circus_message_visitor_query_version_fn)visit_query_version,
};

static void impl_mh_read(circus_channel_t *channel, impl_mh_t *this) {
   if (this->reply == NULL) {
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

      cad_input_stream_t *in = new_cad_input_stream_from_string(buf, this->memory);
      if (in == NULL) {
         log_error(this->log, "message_handler", "Could not allocate input stream");
      } else {
         json_value_t *jmsg = json_parse(in, NULL, this, this->memory);
         if (jmsg == NULL) {
            log_error(this->log, "message_handler", "Could not parse JSON");
         } else {
            circus_message_t *msg = deserialize_circus_message(this->memory, (json_object_t*)jmsg); // TODO what if not an object?
            if (msg == NULL) {
               log_error(this->log, "message_handler", "Could not deserialize message");
            } else {
               msg->accept(msg, (circus_message_visitor_t*)&(this->vfn));
               msg->free(msg);
            }
            jmsg->free(jmsg);
         }
         in->free(in);
      }

      this->memory.free(buf);
   }
}

static void impl_mh_write(circus_channel_t *channel, impl_mh_t *this) {
   char *szout = NULL;
   if (this->reply != NULL) {
      json_object_t *reply = this->reply->serialize(this->reply);
      if (reply == NULL) {
         log_error(this->log, "message_handler", "Could not serialize message");
      } else {
         cad_output_stream_t *out = new_cad_output_stream_from_string(&szout, this->memory);
         if (out == NULL) {
            log_error(this->log, "message_handler", "Could not allocate output stream");
         } else {
            json_visitor_t *writer = json_write_to(out, this->memory, json_compact);
            if (writer == NULL) {
               log_error(this->log, "message_handler", "Could not allocate JSON writer");
            } else {
               reply->accept(reply, writer);
               reply->free(reply);
            }
            writer->free(writer);
         }
         out->free(out);
      }

      if (szout != NULL) {
         channel->write(channel, szout, strlen(szout));
         this->memory.free(szout);
      }

      this->reply->free(this->reply);
      this->reply = NULL;
   }
}

static void impl_register_to(impl_mh_t *this, circus_channel_t *channel) {
   channel->on_read(channel, (circus_channel_on_read_cb)impl_mh_read, this);
   channel->on_write(channel, (circus_channel_on_write_cb)impl_mh_write, this);
   this->running = 1;
}

static void impl_free(impl_mh_t *this) {
   if (this->vault != NULL) {
      this->vault->free(this->vault);
   }
   this->session->free(this->session);
   this->memory.free(this);
}

static circus_server_message_handler_t impl_mh_fn = {
   (circus_server_message_handler_register_to_fn) impl_register_to,
   (circus_server_message_handler_free_fn) impl_free,
};

circus_server_message_handler_t *circus_message_handler(cad_memory_t memory, circus_log_t *log, circus_vault_t *vault, circus_config_t *UNUSED(config)) {
   impl_mh_t *result;

   result = memory.malloc(sizeof(impl_mh_t));
   assert(result != NULL);

   result->fn = impl_mh_fn;
   result->vfn = visitor_fn;
   result->memory = memory;
   result->log = log;
   result->vault = vault;
   result->session = circus_session(memory, log);
   result->reply = NULL;

   assert(result->session != NULL);

   return I(result);
}
