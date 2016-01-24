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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include <sys/mman.h>

#include "channel.h"
#include "circus.h"
#include "config.h"
#include "log.h"

#include "../server/message_handler.h"

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

typedef struct {
   size_t size;
   char data[0];
} mem;

static mem *circus_memalloc(size_t size) {
   size_t s = sizeof(mem) + size;
   mem *result = malloc(s);
   if (result != NULL) {
      int n = mlock(result, s);
      if (n != 0) {
         log_error(LOG, "server", "mlock failed: %s", strerror(errno));
         free(result);
      }
      memset(&(result->data), 0, size);
      result->size = size;
   }
   return result;
}

static void circus_memfree(mem *p) {
   memset(&(p->data), 0, p->size);
   int n = munlock(p, sizeof(mem) + p->size);
   if (n != 0) {
      log_warning(LOG, "server", "munlock failed: %s", strerror(errno));
   }
   free(p);
}

static void *circus_malloc(size_t size) {
   mem *result = circus_memalloc(size);
   if (result == NULL) {
      return NULL;
   }
   return &(result->data);
}

static void *circus_realloc(void *ptr, size_t size) {
   if (ptr == NULL) {
      return circus_malloc(size);
   }
   mem *p = container_of(ptr, mem, data);
   if (size <= p->size) {
      return ptr;
   }
   mem *result = circus_memalloc(size);
   memcpy(&(result->data), &(p->data), p->size);
   circus_memfree(p);
   return &(result->data);
}

static void circus_free(void *ptr) {
   if (ptr != NULL) {
      circus_memfree(container_of(ptr, mem, data));
   }
}

cad_memory_t MEMORY = {circus_malloc, circus_realloc, circus_free};

__PUBLIC__ int main() {
   circus_config_t *config = circus_config_read(stdlib_memory, "server.conf");
   assert(config != NULL);

   set_log(config);
   if (LOG == NULL) {
      config->free(config);
      exit(1);
   }

   circus_channel_t *channel = circus_zmq_server(MEMORY, config);
   if (channel == NULL) {
      log_error(LOG, "server", "Could not allocate channel");
      LOG->free(LOG);
      config->free(config);
      exit(1);
   }
   circus_server_message_handler_t *mh = circus_message_handler(MEMORY, config);
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
