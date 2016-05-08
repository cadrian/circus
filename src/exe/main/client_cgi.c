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

#include <cad_cgi.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include <circus_channel.h>
#include <circus_config.h>
#include <circus_log.h>
#include <circus_memory.h>

#include "../client/message_handler.h"
#include "../client/cgi_handler.h"

static circus_log_t *LOG;

static void set_log(circus_config_t *config) {
   const char *log_szfilename;
   const char *log_szlevel;
   log_level_t log_level = LOG_INFO;

   log_szlevel = config->get(config, "log", "level");
   if (log_szlevel != NULL) {
      if (!strcmp(log_szlevel, "error")) {
         log_level = LOG_ERROR;
      } else if (!strcmp(log_szlevel, "warning")) {
         log_level = LOG_WARNING;
      } else if (!strcmp(log_szlevel, "info")) {
         log_level = LOG_INFO;
      } else if (!strcmp(log_szlevel, "debug")) {
         log_level = LOG_DEBUG;
      } else {
         fprintf(stderr, "Ignored unknown log level: %s\n", log_szlevel);
      }
   }

   log_szfilename = config->get(config, "log", "filename");
   if (log_szfilename == NULL) {
      LOG = circus_new_log_file_descriptor(stdlib_memory, log_level, STDERR_FILENO);
   } else {
      LOG = circus_new_log_file(stdlib_memory, log_szfilename, log_level);
   }
}

static void usage(const char *cmd, FILE *out) {
   fprintf(out,
           "Usage: %s\n"
           "\n"
           "Called without argument, this CGI program follows\n"
           "CGI conventions and looks for relevant data into\n"
           "its environment.\n",
           cmd
   );
}

static void finished(circus_automaton_t *UNUSED(automaton), void *UNUSED(data)) {
   log_debug(LOG, "client_cgi", "Finished");
   uv_stop(uv_default_loop());
}

static void run(circus_config_t *config) {
   circus_channel_t *zmq_channel = circus_zmq_client(MEMORY, LOG, config);
   if (zmq_channel == NULL) {
      log_error(LOG, "client_cgi", "Could not allocate zmq_channel");
      LOG->free(LOG);
      config->free(config);
      exit(1);
   }

   circus_client_message_handler_t *mh = circus_message_handler(MEMORY, LOG, config);
   if (mh == NULL) {
      log_error(LOG, "client_cgi", "Could not allocate message handler");
      zmq_channel->free(zmq_channel);
      LOG->free(LOG);
      config->free(config);
      exit(1);
   }

   circus_channel_t *cgi_channel = circus_cgi(MEMORY, LOG, config);
   if (cgi_channel == NULL) {
      log_error(LOG, "client_cgi", "Could not allocate cgi_channel");
      mh->free(mh);
      LOG->free(LOG);
      config->free(config);
      exit(1);
   }

   circus_client_cgi_handler_t *ch = circus_cgi_handler(MEMORY, LOG, config);
   if (ch == NULL) {
      log_error(LOG, "client_cgi", "Could not allocate CGI handler");
      cgi_channel->free(cgi_channel);
      mh->free(mh);
      zmq_channel->free(zmq_channel);
      LOG->free(LOG);
      config->free(config);
      exit(1);
   }

   circus_automaton_t *automaton = new_automaton(MEMORY, LOG);
   assert(automaton->state(automaton) == State_started);
   mh->register_to(mh, zmq_channel, automaton);
   ch->register_to(ch, cgi_channel, automaton);

   automaton->on_state(automaton, State_finished, finished, NULL);
   automaton->set_state(automaton, State_read_from_client, NULL);

   log_info(LOG, "client_cgi", "Client started.");
   uv_run(uv_default_loop(), UV_RUN_DEFAULT);
   uv_loop_close(uv_default_loop());
   log_info(LOG, "client_cgi", "Client stopped.");

   ch->free(ch);
   cgi_channel->free(cgi_channel);
   mh->free(mh);
   zmq_channel->free(zmq_channel);
}

__PUBLIC__ int main(int argc, const char* const* argv) {
   circus_config_t *config = circus_config_read(stdlib_memory, "cgi.conf");
   int status = 0;

   assert(config != NULL);

   set_log(config);
   if (LOG == NULL) {
      config->free(config);
      exit(1);
   }
   log_info(LOG, "client_cgi", "Client starting.");

   if (argc != 1) {
      usage(argv[0], stderr);
      status = 1;
   } else {
      run(config);
   }

   LOG->free(LOG);
   config->free(config);
   return status;
}
