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

#include <string.h>

#include <circus_xdg.h>

#include "client_impl.h"
#include "gen/web.h"

static void impl_cgi_read(circus_channel_t *UNUSED(channel), impl_cgi_t *this, cad_cgi_response_t *response) {
   if (this->automaton->state(this->automaton) == State_read_from_client) {
      log_info(this->log, "cgi_handler", "CGI read");
      cad_cgi_meta_t *meta = response->meta_variables(response);
      const char *verb = meta->request_method(meta);
      const char *path = meta->path_info(meta);
      log_debug(this->log, "cgi_handler", "verb: %s -- path: \"%s\"", verb, path);
      if (!strcmp(verb, "GET")) {
         if (strcmp(path, "") && strcmp(path, "/")) {
            set_response_string(this, response, 401, "Invalid query\n");
         } else {
            cad_hash_t *query = meta->query_string(meta);
            if (query != NULL && query->count(query) != 0) {
               set_response_string(this, response, 401, "Invalid query\n");
            } else {
               set_response_template(this, response, 200, "login", NULL);
            }
            if (query != NULL) {
               query->free(query);
            }
         }
      } else if (!strcmp(verb, "POST")) {
         post_read(this, response);
      } else {
         set_response_string(this, response, 401, "Invalid query\n");
      }
   }
}

static void impl_cgi_write(circus_channel_t *UNUSED(channel), impl_cgi_t *this, cad_cgi_response_t *response) {
   if (this->automaton->state(this->automaton) == State_write_to_client) {
      log_info(this->log, "cgi_handler", "CGI write");
      cad_cgi_meta_t *meta = response->meta_variables(response);
      const char *verb = meta->request_method(meta);
      if (!strcmp(verb, "GET")) {
         log_debug(this->log, "cgi_handler", "GET: nothing more to write");
      } else if (!strcmp(verb, "POST")) {
         post_write(this, response);
      } else {
         assert(0/* BUG */);
      }
      circus_message_t *msg = this->automaton->message(this->automaton);
      if (msg != NULL) {
         msg->free(msg);
      }
      this->automaton->set_state(this->automaton, State_finished, NULL);
   }
}

static void on_read(circus_automaton_t *automaton, impl_cgi_t *this) {
   assert(this->automaton == automaton);
   log_debug(this->log, "cgi_handler", "register on_read");
   this->channel->on_read(this->channel, (circus_channel_on_read_cb)impl_cgi_read, this);
}

static void on_write(circus_automaton_t *automaton, impl_cgi_t *this) {
   assert(this->automaton == automaton);
   log_debug(this->log, "cgi_handler", "register on_write");
   this->channel->on_write(this->channel, (circus_channel_on_write_cb)impl_cgi_write, this);
}

static void impl_register_to(impl_cgi_t *this, circus_channel_t *channel, circus_automaton_t *automaton) {
   assert(automaton != NULL);
   this->channel = channel;
   this->automaton = automaton;
   this->automaton->on_state(this->automaton, State_read_from_client, (circus_automaton_state_cb)on_read, this);
   this->automaton->on_state(this->automaton, State_write_to_client, (circus_automaton_state_cb)on_write, this);
}

static void impl_free(impl_cgi_t *this) {
   this->memory.free(this->templates_path);
   this->memory.free(this);
}

static circus_client_cgi_handler_t impl_cgi_fn = {
   (circus_client_cgi_handler_register_to_fn) impl_register_to,
   (circus_client_cgi_handler_free_fn) impl_free,
};

circus_client_cgi_handler_t *circus_cgi_handler(cad_memory_t memory, circus_log_t *log, circus_config_t *config) {
   impl_cgi_t *result = memory.malloc(sizeof(impl_cgi_t));
   assert(result != NULL);

   const char *tp = config->get(config, "cgi", "templates_path");
   char *templates_path;
   if (tp == NULL) {
      templates_path = szprintf(memory, NULL, "%s/templates", xdg_config_home());
   } else {
      templates_path = szprintf(memory, NULL, "%s", tp);
   }
   log_info(log, "cg_handler", "templates path: %s", templates_path);

   result->fn = impl_cgi_fn;
   result->memory = memory;
   result->log = log;
   result->channel = NULL;
   result->automaton = NULL;
   result->templates_path = templates_path;

   return I(result);
}

/*
 *  100   Continue
 *  101   Switching Protocols
 *  200   OK
 *  201   Created
 *  202   Accepted
 *  203   Non-Authoritative Information
 *  204   No Content
 *  205   Reset Content
 *  206   Partial Content
 *  300   Multiple Choices
 *  301   Moved Permanently
 *  302   Found
 *  303   See Other
 *  304   Not Modified
 *  305   Use Proxy
 *  307   Temporary Redirect
 *  400   Bad Request
 *  401   Unauthorized
 *  402   Payment Required
 *  403   Forbidden
 *  404   Not Found
 *  405   Method Not Allowed
 *  406   Not Acceptable
 *  407   Proxy Authentication Required
 *  408   Request Time-out
 *  409   Conflict
 *  410   Gone
 *  411   Length Required
 *  412   Precondition Failed
 *  413   Request Entity Too Large
 *  414   Request-URI Too Large
 *  415   Unsupported Media Type
 *  416   Requested range not satisfiable
 *  417   Expectation Failed
 *  500   Internal Server Error
 *  501   Not Implemented
 *  502   Bad Gateway
 *  503   Service Unavailable
 *  504   Gateway Time-out
 *  505   HTTP Version not supported
 */
