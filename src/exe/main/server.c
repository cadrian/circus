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
#include <unistd.h>
#include <uv.h>

#include <circus_channel.h>
#include <circus_config.h>
#include <circus_crypt.h>
#include <circus_log.h>
#include <circus_memory.h>
#include <circus_vault.h>

#include "../server/message_handler.h"

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
      } else if (!strcmp(log_szlevel, "pii")) {
         log_level = LOG_PII;
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
           "Usage: %s [--install <username> <password>|--help]\n"
           "\n"
           "Usually called without arguments; just start the Circus server.\n"
           "\n"
           "Calling with --install is usually done by installation procedures\n"
           "(package managers and so on).\n"
           "It checks that the database exists, is in the good version, and\n"
           "creates the administrator user with the given name and password.\n",
           cmd
   );
}

static void run(circus_config_t *config, circus_vault_t *vault) {
   circus_channel_t *channel = circus_zmq_server(MEMORY, LOG, config);
   if (channel == NULL) {
      log_error(LOG, "Could not allocate channel");
      LOG->free(LOG);
      config->free(config);
      exit(1);
   }

   circus_server_message_handler_t *mh = circus_message_handler(MEMORY, LOG, vault, config);
   if (mh == NULL) {
      log_error(LOG, "Could not allocate message handler");
      channel->free(channel);
      LOG->free(LOG);
      config->free(config);
      exit(1);
   }

   mh->register_to(mh, channel);

   log_info(LOG, "Server started.");
   uv_run(uv_default_loop(), UV_RUN_DEFAULT);
   uv_loop_close(uv_default_loop());
   log_info(LOG, "Server stopped.");

   mh->free(mh);
   channel->free(channel);
}

__PUBLIC__ int main(int argc, const char* const* argv) {
   circus_config_t *config = circus_config_read(stdlib_memory, "server.conf");
   int status = 0;

   assert(config != NULL);

   set_log(config);
   if (LOG == NULL) {
      config->free(config);
      exit(1);
   }
   log_info(LOG, "Server starting.");

   if (!init_crypt(LOG)) {
      status = 1;
   } else {
      circus_vault_t *vault = circus_vault(MEMORY, LOG, config, circus_database_sqlite3);
      switch (argc) {
      case 1:
         run(config, vault);
         assert(0 == status);
         break;
      case 2:
         if (0 == strcmp("--help", argv[1])) {
            usage(argv[0], stdout);
         } else {
            usage(argv[0], stderr);
            status = 1;
         }
         break;
      case 4:
         if (0 == strcmp("--install", argv[1])) {
            status = vault->install(vault, argv[2], argv[3]);
         } else {
            usage(argv[0], stderr);
            status = 1;
         }
         break;
      default:
         usage(argv[0], stderr);
         status = 1;
         break;
      }
   }

   LOG->free(LOG);
   config->free(config);
   return status;
}
