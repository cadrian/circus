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

#include <cad_array.h>
#include <cad_hash.h>
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
   cad_array_t *nested;
};

struct meta_resolved_string {
   cad_stache_resolved_t fn;
   cad_memory_t memory;
   const char *value;
};

static const char *meta_resolved_string_get(struct meta_resolved_string *this) {
   return this->value;
}

static int meta_resolved_string_free(struct meta_resolved_string *this) {
   this->memory.free(this);
   return 1;
}

static cad_stache_resolved_t meta_resolved_string_fn = {
   .string = {
      .get = (const char*(*)(cad_stache_resolved_t*))meta_resolved_string_get,
      .free = (int (*)(cad_stache_resolved_t*))meta_resolved_string_free,
   },
};

struct meta_resolved_strings {
   cad_stache_resolved_t fn;
   cad_memory_t memory;
   cad_array_t *value;
   unsigned int meta_index;
   struct meta_data *meta;
};

int meta_resolved_strings_get(struct meta_resolved_strings *this, unsigned int index) {
   int result = 0;
   if (index < this->value->count(this->value)) {
      cad_hash_t *hash = this->meta->nested->get(this->meta->nested, this->meta_index);
      char *item = this->value->get(this->value, index);
      hash->set(hash, "item", item);
      result = 1;
   }
   return result;
}

int meta_resolved_strings_close(struct meta_resolved_strings *this) {
   assert(this->meta_index == this->meta->nested->count(this->meta->nested) - 1);
   cad_hash_t *hash = this->meta->nested->del(this->meta->nested, this->meta_index);
   hash->free(hash);
   this->memory.free(this);
   return 1;
}

static cad_stache_resolved_t meta_resolved_strings_fn = {
   .list = {
      .get = (int(*)(cad_stache_resolved_t *this, int index))meta_resolved_strings_get,
      .close = (int(*)(cad_stache_resolved_t *this))meta_resolved_strings_close,
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

      const char *sz_value = dict->get(dict, key);
      if (sz_value != NULL) {
         struct meta_resolved_string *res = meta->memory.malloc(sizeof(struct meta_resolved_string));
         assert(res != NULL);
         res->fn = meta_resolved_string_fn;
         res->memory = meta->memory;
         res->value = sz_value;
         *resolved = I(res);
         return Cad_stache_string;
      }

      char *akey = szprintf(meta->memory, NULL, "#%s", key);
      cad_array_t *ar_value = dict->get(dict, key);
      meta->memory.free(akey);
      if (ar_value != NULL) {
         struct meta_resolved_strings *res = meta->memory.malloc(sizeof(struct meta_resolved_strings));
         assert(res != NULL);
         unsigned int meta_index = meta->nested->count(meta->nested);
         cad_hash_t *hash = cad_new_hash(meta->memory, cad_hash_strings);
         meta->nested->insert(meta->nested, meta_index, hash);
         res->fn = meta_resolved_strings_fn;
         res->memory = meta->memory;
         res->value = ar_value;
         res->meta_index = meta_index;
         res->meta = meta;
         *resolved = I(res);
         return Cad_stache_list;
      }

   }
   return Cad_stache_not_found;
}

static void response_security_headers(cad_cgi_response_t *response) {
   response->set_header(response, "X-Frame-Options", "SAMEORIGIN");
   response->set_header(response, "Cache-Control", "private, no-cache, no-store");
   response->set_header(response, "Pragma", "no-cache");
   response->set_header(response, "Expires", "0");
}

void set_response_string(impl_cgi_t *this, cad_cgi_response_t *response, int status, const char *string) {
   cad_output_stream_t *body = response->body(response);
   log_debug(this->log, "resolve", "response string: status %d -- %s", status, string);
   response->set_status(response, status);
   body->put(body, string);
   response_security_headers(response);
   this->automaton->set_state(this->automaton, State_write_to_client, NULL);
}

static void template_error(const char *error, int offset, void *data) {
   impl_cgi_t *this = (impl_cgi_t*)data;
   log_error(this->log, "resolve", "Stache error: %s at %d", error, offset);
}

void set_response_template(impl_cgi_t *this, cad_cgi_response_t *response, int status, const char *template, cad_hash_t *extra) {
   cad_output_stream_t *body = response->body(response);
   cad_cgi_meta_t *meta = response->meta_variables(response);
   cad_array_t *nested = cad_new_array(this->memory, sizeof(cad_hash_t*));
   struct meta_data data = {meta, extra, this->memory, nested};
   char *template_path = szprintf(this->memory, NULL, "%s/%s.tpl", this->templates_path, template);
   assert(template_path != NULL);
   int template_fd = open(template_path, O_RDONLY);
   if (template_fd == -1) {
      log_error(this->log, "resolve", "Error opening template: %s", template_path);
      set_response_string(this, response, 500, "Server error\n");
   } else {
      log_debug(this->log, "resolve", "status: %d -- template: %s", status, template_path);
      response->set_content_type(response, "text/html"); // TODO config
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
      assert(nested->count(nested) == 0);
      nested->free(nested);
      response_security_headers(response);
      this->automaton->set_state(this->automaton, State_write_to_client, NULL);
   }
   this->memory.free(template_path);
}
