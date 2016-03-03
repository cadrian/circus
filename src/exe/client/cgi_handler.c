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

#include <circus_message_impl.h>
#include <circus_xdg.h>

#include "client_impl.h"

static void impl_cgi_read(circus_channel_t *UNUSED(channel), impl_cgi_t *this, cad_cgi_response_t *response) {
   if (this->automaton->state(this->automaton) == State_read_from_client) {
      cad_cgi_meta_t *meta = response->meta_variables(response);
      const char *verb = meta->request_method(meta);
      const char *path = meta->path_info(meta);
      if (!strcmp(verb, "GET")) {
         if (strcmp(path, "") || strcmp(path, "/")) {
            set_response_string(this, response, 401, "Invalid path");
         } else {
            cad_hash_t *query = meta->query_string(meta);
            assert(query != NULL);
            if (query->count(query) != 0) {
               set_response_string(this, response, 401, "Invalid query");
            } else {
               set_response_template(this, response, 200, "login", NULL);
            }
            query->free(query);
         }
      } else if (!strcmp(verb, "POST")) {
         circus_message_t *message = NULL;
         cad_hash_t *form = meta->input_as_form(meta);
         assert(form != NULL);

         if (!strcmp(path, "/login.do")) {
            const char *username = form->get(form, "username");
            const char *password = form->get(form, "password");
            message = I(new_circus_message_query_login(this->memory, username, password));
         }

         if (message == NULL) {
            set_response_string(this, response, 401, "Invalid path");
         } else {
            this->automaton->set_state(this->automaton, State_write_to_server, message);
         }
      } else {
         set_response_string(this, response, 405, "Unknown method");
      }
   }
}

static void impl_cgi_write(circus_channel_t *UNUSED(channel), impl_cgi_t *this, cad_cgi_response_t *response) {
   if (this->automaton->state(this->automaton) == State_write_to_client) {
      cad_cgi_meta_t *meta = response->meta_variables(response);
      const char *verb = meta->request_method(meta);
      if (!strcmp(verb, "POST")) {
         const char *path = meta->path_info(meta);
         if (strcmp(path, "/login.do")) {
            circus_message_reply_login_t *login = (circus_message_reply_login_t*)this->automaton->message(this->automaton);
            const char *error = I(login)->error(I(login));
            if (strlen(error) == 0) {
               set_response_template(this, response, 200, "home", NULL);
            } else {
               log_error(this->log, "cgi_handler", "login error: %s", error);
               set_response_string(this, response, 403, "Access denied.");
            }
         }
      }
      int n = response->flush(response);
      assert(n == 0);
   }
}

static void impl_register_to(impl_cgi_t *this, circus_channel_t *channel, circus_automaton_t *automaton) {
   assert(automaton != NULL);
   this->automaton = automaton;
   channel->on_read(channel, (circus_channel_on_read_cb)impl_cgi_read, this);
   channel->on_write(channel, (circus_channel_on_write_cb)impl_cgi_write, this);
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

   result->fn = impl_cgi_fn;
   result->memory = memory;
   result->log = log;
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
