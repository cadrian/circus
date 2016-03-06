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

#include <cad_stache.h>
#include <cad_stream.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "client_impl.h"

struct meta_data {
   cad_cgi_meta_t *data;
   cad_hash_t *extra;
   cad_memory_t memory;
};

struct meta_resolved {
   cad_stache_resolved_t fn;
   cad_memory_t memory;
   const char *value;
};

static const char *meta_resolved_get(struct meta_resolved *this) {
   return this->value;
}

static int meta_resolved_free(struct meta_resolved *this) {
   this->memory.free(this);
   return 1;
}

static cad_stache_resolved_t meta_resolved_fn = {
   .string = {
      .get = (const char*(*)(cad_stache_resolved_t*))meta_resolved_get,
      .free = (int (*)(cad_stache_resolved_t*))meta_resolved_free,
   },
};

static cad_stache_lookup_type resolve_meta(cad_stache_t *UNUSED(stache), const char *name, struct meta_data *meta, cad_stache_resolved_t **resolved) {
   cad_hash_t *dict = NULL;
   const char *key = NULL;
   if (!strncmp(name, "form:", 5)) {
      dict = meta->data->input_as_form(meta->data);
      key = name + 5;
   } else if (!strncmp(name, "query:", 6)) {
      dict = meta->data->query_string(meta->data);
      key = name + 6;
   } else if (strchr(name, ':') == NULL) {
      dict = meta->extra;
      key = name;
   }
   if (dict != NULL) {
      assert(key != NULL);
      const char *value = dict->get(dict, key);
      if (value != NULL) {
         struct meta_resolved *res = meta->memory.malloc(sizeof(struct meta_resolved));
         assert(res != NULL);
         res->fn = meta_resolved_fn;
         res->memory = meta->memory;
         res->value = value;
         *resolved = I(res);
         return Cad_stache_string;
      }
   }
   return Cad_stache_not_found;
}

void set_response_string(impl_cgi_t *this, cad_cgi_response_t *response, int status, const char *string) {
   cad_output_stream_t *body = response->body(response);
   log_debug(this->log, "resolve", "response string: status %d -- %s", status, string);
   response->set_status(response, status);
   body->put(body, string);
   this->automaton->set_state(this->automaton, State_write_to_client, NULL);
}

static void template_error(const char *error, int offset, void *data) {
   impl_cgi_t *this = (impl_cgi_t*)data;
   log_error(this->log, "Stache error: %s at %d", error, offset);
}

void set_response_template(impl_cgi_t *this, cad_cgi_response_t *response, int status, const char *template, cad_hash_t *extra) {
   cad_output_stream_t *body = response->body(response);
   cad_cgi_meta_t *meta = response->meta_variables(response);
   struct meta_data data = {meta, extra, this->memory};
   char *template_path = szprintf(this->memory, NULL, "%s/%s.tpl", this->templates_path, template);
   assert(template_path != NULL);
   int template_fd = open(template_path, O_RDONLY);
   if (template_fd == -1) {
      log_error(this->log, "resolve", "Error opening template: %s", template_path);
      set_response_string(this, response, 500, "Error opening template");
   } else {
      response->set_status(response, status);
      cad_input_stream_t *in = new_cad_input_stream_from_file_descriptor(template_fd, this->memory);
      assert(in != NULL);
      cad_stache_t *stache = new_cad_stache(this->memory, (cad_stache_resolve_cb)resolve_meta, &data);
      assert(stache != NULL);
      log_debug(this->log, "resolve", "Rendering stache...");
      stache->render(stache, in, body, template_error, this);
      stache->free(stache);
      in->free(in);
      close(template_fd);
      this->automaton->set_state(this->automaton, State_write_to_client, NULL);
   }
   this->memory.free(template_path);
}
