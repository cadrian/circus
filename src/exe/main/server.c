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
*/

#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "channel.h"
#include "circus.h"
#include "config.h"
#include "log.h"

circus_log_t *LOG;

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
      LOG = circus_new_log_stdout(stdlib_memory, log_level);
   } else {
      LOG = circus_new_log_file(stdlib_memory, log_szfilename, log_level);
   }
}

__PUBLIC__ int main() {
   circus_config_t *config = circus_config_read(stdlib_memory, "server.conf");
   set_log(config);

   circus_channel_t *channel = circus_zmq_server(stdlib_memory, config);

   log_info(LOG, "server", "Server started.");
   uv_run(uv_default_loop(), UV_RUN_DEFAULT);
   uv_loop_close(uv_default_loop());
   log_info(LOG, "server", "Server stopped.");

   channel->free(channel);
   LOG->free(LOG);
   config->free(config);
   return 0;
}
