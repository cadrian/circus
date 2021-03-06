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

#include <string.h>

#include <circus_xdg.h>

#include "client_impl.h"
#include "gen/web.h"

static void debug_form(cad_hash_t *UNUSED(hash), int index, const char *key, const char *value, impl_cgi_t *this) {
   log_pii(this->log, "#%d: %s = %s", index, key, value);
}

static void debug_cookie(cad_cgi_cookies_t *UNUSED(jar), cad_cgi_cookie_t *cookie, impl_cgi_t *this) {
   log_pii(this->log, "COOKIE: %s = %s", cookie->name(cookie), cookie->value(cookie));
}

static void impl_cgi_read(circus_channel_t *UNUSED(channel), impl_cgi_t *this, cad_cgi_response_t *response) {
   SET_CANARY();
   if (this->automaton->state(this->automaton) == State_read_from_client) {
      log_info(this->log, "CGI read");
      cad_cgi_meta_t *meta = response->meta_variables(response);
      const char *verb = meta->request_method(meta);
      const char *path = meta->path_info(meta);
      log_debug(this->log, "verb: %s -- path: \"%s\"", verb, path);
      if (!strcmp(verb, "GET")) {
         if (strcmp(path, "") && strcmp(path, "/")) {
            get_read(this, response);
         } else {
            cad_hash_t *query = meta->query_string(meta);
            if (query != NULL && query->count(query) != 0) {
               set_response_string(this, response, 401, "Invalid query\n");
            } else {
               set_response_redirect(this, response, "login", NULL);
            }
         }
      } else if (!strcmp(verb, "POST")) {
         if (log_is_pii(this->log)) {
            cad_hash_t *form = meta->input_as_form(meta);
            log_pii(this->log, "Listing form parameters:");
            form->iterate(form, (cad_hash_iterator_fn)debug_form, this);
            cad_cgi_cookies_t *cookies = response->cookies(response);
            log_pii(this->log, "Listing cookies:");
            cookies->iterate(cookies, (cad_cgi_cookie_iterator_fn)debug_cookie, this);
            log_pii(this->log, "End of listing.");
         }
         post_read(this, response);
      } else {
         set_response_string(this, response, 401, "Invalid query\n");
      }
   }
   CHECK_CANARY();
}

static void impl_cgi_write(circus_channel_t *UNUSED(channel), impl_cgi_t *this, cad_cgi_response_t *response) {
   SET_CANARY();
   if (this->automaton->state(this->automaton) == State_write_to_client) {
      log_info(this->log, "CGI write");
      cad_cgi_meta_t *meta = response->meta_variables(response);
      const char *verb = meta->request_method(meta);
      if (!strcmp(verb, "GET")) {
         get_write(this, response);
      } else if (!strcmp(verb, "POST")) {
         post_write(this, response);
      } else {
         assert(0/* BUG */);
      }
      circus_message_t *msg = this->automaton->message(this->automaton);
      if (msg != NULL) {
         msg->free(msg);
      }
   }
   CHECK_CANARY();
}

static void impl_cgi_write_done(circus_channel_t *UNUSED(channel), impl_cgi_t *this) {
   this->automaton->set_state(this->automaton, State_finished, NULL);
}

static void on_read(circus_automaton_t *automaton, impl_cgi_t *this) {
   assert(this->automaton == automaton);
   log_debug(this->log, "register on_read");
   this->channel->on_read(this->channel, (circus_channel_on_read_cb)impl_cgi_read, this);
}

static void on_write(circus_automaton_t *automaton, impl_cgi_t *this) {
   assert(this->automaton == automaton);
   log_debug(this->log, "register on_write");
   this->channel->on_write(this->channel, (circus_channel_on_write_cb)impl_cgi_write, (circus_channel_on_write_done_cb)impl_cgi_write_done, this);
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
      read_t read = read_xdg_file_from_dirs(memory, "templates", xdg_config_dirs());
      templates_path = read.path;
      if (read.file != NULL) {
         int n = fclose(read.file);
         assert(n == 0);
      }
   } else {
      templates_path = szprintf(memory, NULL, "%s", tp);
   }
   log_info(log, "templates path: %s", templates_path);

   result->fn = impl_cgi_fn;
   result->memory = memory;
   result->log = log;
   result->config = config;
   result->channel = NULL;
   result->automaton = NULL;
   result->templates_path = templates_path;
   const char *secure = config->get(config, "cgi", "secure");
   if (secure == NULL || strcmp(secure, "No: Test")) {
      result->cookie_flag = Cookie_secure | Cookie_http_only;
   }
   result->ready = 0;

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
