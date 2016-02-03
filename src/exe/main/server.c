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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include <circus_channel.h>
#include <circus_config.h>
#include <circus_log.h>
#include <circus_memory.h>

#include "../server/message_handler.h"

static circus_log_t *LOG;

static void set_log(circus_config_t *config) {
   const char *log_szfilename = config->get(config, "log", "filename");
   const char *log_szlevel = config->get(config, "log", "level");
   log_level_t log_level = LOG_INFO;
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
   if (log_szfilename == NULL) {
      LOG = circus_new_log_stderr(stdlib_memory, log_level);
   } else {
      LOG = circus_new_log_file(stdlib_memory, log_szfilename, log_level);
   }
}

__PUBLIC__ int main() {
   circus_config_t *config = circus_config_read(stdlib_memory, LOG, "server.conf");
   assert(config != NULL);

   set_log(config);
   if (LOG == NULL) {
      config->free(config);
      exit(1);
   }

   circus_channel_t *channel = circus_zmq_server(MEMORY, LOG, config);
   if (channel == NULL) {
      log_error(LOG, "server", "Could not allocate channel");
      LOG->free(LOG);
      config->free(config);
      exit(1);
   }
   circus_server_message_handler_t *mh = circus_message_handler(MEMORY, LOG, config);
   if (mh == NULL) {
      log_error(LOG, "server", "Could not allocate message handler");
      channel->free(channel);
      LOG->free(LOG);
      config->free(config);
      exit(1);
   }

   mh->register_to(mh, channel);

   log_info(LOG, "server", "Server started.");
   uv_run(uv_default_loop(), UV_RUN_DEFAULT);
   uv_loop_close(uv_default_loop());
   log_info(LOG, "server", "Server stopped.");

   mh->free(mh);
   channel->free(channel);
   LOG->free(LOG);
   config->free(config);
   return 0;
}
