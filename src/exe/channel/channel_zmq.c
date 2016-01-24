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

#include <string.h>
#include <uv.h>
#include <zmq.h>

#include <circus_channel.h>
#include <circus_log.h>

extern circus_log_t *LOG;

typedef struct {
   circus_channel_t fn;
   cad_memory_t memory;
   void *context;
   void *socket;
   char *addr;
   uv_poll_t handle;
   circus_channel_on_read_cb read_cb;
   void *read_data;
   circus_channel_on_write_cb write_cb;
   void *write_data;
} zmq_impl_t;

static void impl_on_read(zmq_impl_t *this, circus_channel_on_read_cb cb, void *data) {
   this->read_cb = cb;
   this->read_data = data;
}

static void impl_on_write(zmq_impl_t *this, circus_channel_on_write_cb cb, void *data) {
   this->write_cb = cb;
   this->write_data = data;
}

static int impl_read(zmq_impl_t *this, char *buffer, size_t buflen) {
   zmq_msg_t request;
   zmq_msg_init_size(&request, buflen);
   int result = zmq_msg_recv(&request, this->socket, 0);
   if (result > 0) {
      memcpy(zmq_msg_data(&request), buffer, result);
   }
   zmq_msg_close(&request);
   return result;
}

static void impl_write(zmq_impl_t *this, const char *buffer, size_t buflen) {
   zmq_msg_t reply;
   zmq_msg_init_size(&reply, buflen);
   memcpy(zmq_msg_data(&reply), buffer, buflen);
   zmq_msg_send(&reply, this->socket, 0);
   zmq_msg_close(&reply);
}

static void impl_free(zmq_impl_t *this) {
   uv_poll_stop(&(this->handle));

   zmq_close(this->socket);
   zmq_ctx_destroy(this->context);

   this->memory.free(this->addr);
   this->memory.free(this);
}

circus_channel_t impl_fn = {
   (circus_channel_on_read_fn)impl_on_read,
   (circus_channel_on_write_fn)impl_on_write,
   (circus_channel_read_fn)impl_read,
   (circus_channel_write_fn)impl_write,
   (circus_channel_free_fn)impl_free,
};

static void impl_zmq_callback(uv_poll_t *handle, int status, int events) {
   zmq_impl_t *this = container_of(handle, zmq_impl_t, handle);
   if (status != 0) {
      log_warning(LOG, "channel_zmq", "impl_zmq_callback: status=%d", status);
      return;
   }

   if (events & UV_READABLE) {
      uint32_t zevents;
      size_t zevents_size;
      int n = zmq_getsockopt(this->socket, ZMQ_EVENTS, &zevents, &zevents_size);
      if (n < 0) {
         fprintf(stderr, "Error %d while getting socket events -- %s\n", zmq_errno(), zmq_strerror(zmq_errno()));
         crash();
      } else {
         assert(zevents_size = sizeof(uint32_t));

         if (zevents & ZMQ_POLLIN) {
            if (this->read_cb != NULL) {
               (this->read_cb)((circus_channel_t*)this, this->read_data);
            }
         }

         if (zevents & ZMQ_POLLOUT) {
            if (this->write_cb != NULL) {
               (this->write_cb)((circus_channel_t*)this, this->write_data);
            }
         }
      }
   }
}

static char *getaddr(cad_memory_t memory, circus_config_t *config, const char *config_ip_name) {
   static int check = 0;
   if (!check) {
      check = 1;
      int major, minor, patch;
      zmq_version(&major, &minor, &patch);
      if (ZMQ_MAKE_VERSION(major, minor, patch) < ZMQ_VERSION) {
         fprintf(stderr, "zmq version is too old: %d.%d.%d - expecting at least %d.%d.%d\n",
                 major, minor, patch,
                 ZMQ_VERSION_MAJOR, ZMQ_VERSION_MINOR, ZMQ_VERSION_PATCH);
         exit(1);
      }
   }

   const char *host_szprotocol = config->get(config, "host", "protocol");
   if (host_szprotocol == NULL) {
      host_szprotocol = "tcp";
   }
   const char *host_szip = config->get(config, "host", config_ip_name);
   if (host_szip == NULL) {
      host_szip = "127.0.0.1";
   }
   const char *host_szport = config->get(config, "host", "port");
   long host_iport = host_szport == NULL ? 0 : strtol(host_szport, NULL, 10);
   if (host_iport <= 0 || host_iport > 65535) {
      host_iport = DEFAULT_PORT;
   }

   char *addr = szprintf(memory, NULL, "%s://%s:%ld", host_szprotocol, host_szip, host_iport);
   assert(addr != NULL);

   return addr;
}

static void start(zmq_impl_t *this) {
   int fd, n;
   size_t fd_size;

   n = zmq_getsockopt(this->socket, ZMQ_FD, &fd, &fd_size);
   if (n < 0) {
      fprintf(stderr, "Error %d while getting socket fd -- %s\n", zmq_errno(), zmq_strerror(zmq_errno()));
      crash();
   } else {
      assert(fd_size = sizeof(int));

      n = uv_poll_init(uv_default_loop(), &(this->handle), fd);
      assert(n == 0);

      n = uv_poll_start(&(this->handle), UV_READABLE, impl_zmq_callback);
      assert(n == 0);
   }
}

circus_channel_t *circus_zmq_server(cad_memory_t memory, circus_config_t *config) {
   zmq_impl_t *result;

   void *zmq_context = zmq_ctx_new();
   assert(zmq_context != NULL);

   void *zmq_sock = zmq_socket(zmq_context, ZMQ_REP);
   assert(zmq_sock != NULL);

   char *addr = getaddr(memory, config, "bind_ip");

   int rc = zmq_bind(zmq_sock, addr);
   if (rc != 0) {
      fprintf(stderr, "Error %d while binding to %s -- %s\n", zmq_errno(), addr, zmq_strerror(zmq_errno()));
      crash();
   } else {
      result = malloc(sizeof(zmq_impl_t));
      if (result == NULL) {
         log_error(LOG, "channel_zmq", "Could not malloc zmq_server");
      } else {
         result->fn = impl_fn;
         result->memory = memory;
         result->context = zmq_context;
         result->socket = zmq_sock;
         result->addr = addr;

         start(result);
      }
   }

   return (circus_channel_t*)result;
}

circus_channel_t *circus_zmq_client(cad_memory_t memory, circus_config_t *config) {
   zmq_impl_t *result;

   void *zmq_context = zmq_ctx_new();
   assert(zmq_context != NULL);

   void *zmq_sock = zmq_socket(zmq_context, ZMQ_REQ);
   assert(zmq_sock != NULL);

   char *addr = getaddr(memory, config, "connect_ip");

   int rc = zmq_connect(zmq_sock, addr);
   if (rc != 0) {
      fprintf(stderr, "Error %d while connecting to %s -- %s\n", zmq_errno(), addr, zmq_strerror(zmq_errno()));
      crash();
   } else {
      result = malloc(sizeof(zmq_impl_t));
      if (result == NULL) {
         log_error(LOG, "channel_zmq", "Could not malloc zmq_server");
      } else {
         result->fn = impl_fn;
         result->memory = memory;
         result->context = zmq_context;
         result->socket = zmq_sock;
         result->addr = addr;

         start(result);
      }
   }

   return (circus_channel_t*)result;
}
