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

typedef char *(*encode_fn)(cad_memory_t memory, const char *in);

struct meta_data {
   circus_log_t *log;
   cad_cgi_meta_t *data;
   cad_hash_t *extra;
   cad_memory_t memory;
   cad_array_t *nested;
   cad_hash_t *cgi;
   circus_config_t *config;
   encode_fn encode;
};

struct meta_resolved_string {
   cad_stache_resolved_t fn;
   cad_memory_t memory;
   char *value;
};

static char HEX[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

static char *encode_url(cad_memory_t memory, const char *in) {
   size_t len = strlen(in), outlen = len;
   size_t index, outindex;
   char *result;
   char c;
   for (index = 0; index < len; index++) {
      c = in[index];
      if ((c < '0' || c > '9') && (c < 'A' || c > 'Z') && (c < 'a' || c > 'z')) {
         outlen += 2;
      }
   }
   result = memory.malloc(outlen + 1);
   assert(result != NULL);
   for (index = 0, outindex = 0; index < len; index++) {
      c = in[index];
      if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
         result[outindex++] = c;
      } else {
         int lo, hi;
         lo = c & 0x0f;
         hi = (c >> 4) & 0x0f;
         result[outindex++] = '%';
         result[outindex++] = HEX[hi];
         result[outindex++] = HEX[lo];
      }
   }
   result[outlen] = '\0';
   return result;
}

static const char *meta_resolved_string_get(struct meta_resolved_string *this) {
   return this->value;
}

static int meta_resolved_string_free(struct meta_resolved_string *this) {
   this->memory.free(this->value);
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
   circus_log_t *log;
   cad_array_t *value;
   unsigned int meta_index;
   struct meta_data *meta;
};

int meta_resolved_strings_get(struct meta_resolved_strings *this, unsigned int index) {
   int result = 0;
   if (index < this->value->count(this->value)) {
      char *item = this->value->get(this->value, index);
      cad_hash_t *hash = *(cad_hash_t**)this->meta->nested->get(this->meta->nested, this->meta_index);
      hash->set(hash, "item", item);
      result = 1;
   }
   return result;
}

int meta_resolved_strings_close(struct meta_resolved_strings *this) {
   assert(this->meta_index == this->meta->nested->count(this->meta->nested) - 1);
   cad_hash_t *hash = *(cad_hash_t**)this->meta->nested->del(this->meta->nested, this->meta_index);
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
   cad_stache_lookup_type result = Cad_stache_not_found;
   cad_hash_t *dict = NULL;
   char *key = NULL;
   char *operator = NULL;
   log_debug(meta->log, "Resolving %s...", name);

   if (!strncmp(name, "form:", 5)) {
      log_debug(meta->log, " -> input form variable");
      dict = meta->data->input_as_form(meta->data);
      key = szprintf(meta->memory, NULL, "%s", name + 5);
   } else if (!strncmp(name, "query:", 6)) {
      log_debug(meta->log, " -> input query variable");
      dict = meta->data->query_string(meta->data);
      key = szprintf(meta->memory, NULL, "%s", name + 6);
   } else if (!strncmp(name, "cgi:", 4)) {
      log_debug(meta->log, " -> CGI variable");
      dict = meta->cgi;
      key = szprintf(meta->memory, NULL, "%s", name + 4);
   } else if (strchr(name, ':') == NULL) {
      int n = meta->nested->count(meta->nested);
      if (n > 0) {
         log_debug(meta->log, " -> reply variable, nested #%d", n);
         dict = *(cad_hash_t**)meta->nested->get(meta->nested, n - 1);
      } else {
         log_debug(meta->log, " -> reply variable");
         dict = meta->extra;
      }
      key = szprintf(meta->memory, NULL, "%s", name);
   }

   if (dict != NULL) {
      assert(key != NULL);

      operator = strchr(key, '.');
      if (operator != NULL) {
         *operator++ = '\0';
      }

      log_debug(meta->log, "Trying %s => %s...", name, key);
      const char *sz_value = dict->get(dict, key);
      if (sz_value != NULL) {
         struct meta_resolved_string *res = meta->memory.malloc(sizeof(struct meta_resolved_string));
         assert(res != NULL);
         res->fn = meta_resolved_string_fn;
         res->memory = meta->memory;
         char *op;
         if (operator != NULL && !strcmp("count", operator)) {
            res->value = szprintf(meta->memory, NULL, "%zu", strlen(sz_value));
            op = " (operator)";
         } else if (meta->encode != NULL) {
            res->value = meta->encode(meta->memory, sz_value);
            op = " (encoded)";
         } else {
            res->value = szprintf(meta->memory, NULL, "%s", sz_value);
            op = "";
         }
         log_debug(meta->log, "Resolved %s (%s) as%s string: %s", key, name, op, res->value);
         *resolved = I(res);
         result = Cad_stache_string;
      } else {
         char *akey = szprintf(meta->memory, NULL, "#%s", key);
         log_debug(meta->log, "Trying %s => %s...", name, akey);
         cad_array_t *ar_value = dict->get(dict, akey);
         meta->memory.free(akey);
         if (ar_value != NULL) {
            if (operator != NULL && !strcmp("count", operator)) {
               struct meta_resolved_string *res = meta->memory.malloc(sizeof(struct meta_resolved_string));
               assert(res != NULL);
               res->fn = meta_resolved_string_fn;
               res->memory = meta->memory;
               res->value = szprintf(meta->memory, NULL, "%d", ar_value->count(ar_value));
               log_debug(meta->log, "Resolved %s (%s) as string: %s", key, name, res->value);
               *resolved = I(res);
               result = Cad_stache_string;
            } else {
               struct meta_resolved_strings *res = meta->memory.malloc(sizeof(struct meta_resolved_strings));
               assert(res != NULL);
               unsigned int meta_index = meta->nested->count(meta->nested);
               cad_hash_t *hash = cad_new_hash(meta->memory, cad_hash_strings);
               meta->nested->insert(meta->nested, meta_index, &hash);
               res->fn = meta_resolved_strings_fn;
               res->memory = meta->memory;
               res->log = meta->log;
               res->value = ar_value;
               res->meta_index = meta_index;
               res->meta = meta;
               *resolved = I(res);
               log_debug(meta->log, "Resolved %s (%s) as list: %p / %p", key, name, res, res->value);
               result = Cad_stache_list;
            }
         }
      }
   } else if (!strncmp(name, "config:", 7)) {
      log_debug(meta->log, " -> Config variable");
      key = szprintf(meta->memory, NULL, "%s", name + 7);
      const char *sz_value = meta->config->get(meta->config, "variables", key);
      if (sz_value != NULL) {
         struct meta_resolved_string *res = meta->memory.malloc(sizeof(struct meta_resolved_string));
         assert(res != NULL);
         res->fn = meta_resolved_string_fn;
         res->memory = meta->memory;
         char *op;
         if (operator != NULL && !strcmp("count", operator)) {
            res->value = szprintf(meta->memory, NULL, "%zu", strlen(sz_value));
            op = " (operator)";
         } else if (meta->encode != NULL) {
            res->value = meta->encode(meta->memory, sz_value);
            op = " (encoded)";
         } else {
            res->value = szprintf(meta->memory, NULL, "%s", sz_value);
            op="";
         }
         log_debug(meta->log, "Resolved %s (%s) as%s string: %s", key, name, op, res->value);
         *resolved = I(res);
         result = Cad_stache_string;
      }
   }

   if (result == Cad_stache_not_found) {
      log_debug(meta->log, "Resolved %s (%s) as not found", key, name);
   }
   meta->memory.free(key);
   return result;
}

static void response_security_headers(cad_cgi_response_t *response) {
   response->set_header(response, "X-Frame-Options", "SAMEORIGIN");
   response->set_header(response, "Cache-Control", "no-cache, no-store");
   response->set_header(response, "Pragma", "no-cache");
   response->set_header(response, "Expires", "0");
   response->set_header(response, "Strict-Transport-Security", "max-age=31536000; includeSubDomains");
}

void set_response_string(impl_cgi_t *this, cad_cgi_response_t *response, int status, const char *string) {
   assert(!this->ready);
   cad_output_stream_t *body = response->body(response);
   log_debug(this->log, "response string: status %d -- %s", status, string);
   response->set_status(response, status);
   body->put(body, "%s", string);
   response_security_headers(response);
   this->ready = 1;
   this->automaton->set_state(this->automaton, State_write_to_client, NULL);
}

static void template_error(const char *error, int offset, void *data) {
   impl_cgi_t *this = (impl_cgi_t*)data;
   log_error(this->log, "Stache error: %s at %d", error, offset);
}

static char *resolve_string(impl_cgi_t *this, const char *string, cad_cgi_meta_t *meta, cad_hash_t *extra, encode_fn encode) {
   char *result = NULL;
   cad_array_t *nested = cad_new_array(this->memory, sizeof(cad_hash_t*));
   struct meta_data data = {this->log, meta, extra, this->memory, nested, NULL, this->config, encode};
   cad_input_stream_t *in = new_cad_input_stream_from_string(string, this->memory);
   cad_output_stream_t *out = new_cad_output_stream_from_string(&result, this->memory);
   assert(in != NULL);
   cad_stache_t *stache = new_cad_stache(this->memory, (cad_stache_resolve_cb)resolve_meta, &data);
   assert(stache != NULL);
   log_debug(this->log, "Rendering string");
   stache->render(stache, in, out, template_error, this);
   stache->free(stache);
   out->free(out);
   in->free(in);
   assert(nested->count(nested) == 0);
   nested->free(nested);
   return result;
}

void set_response_redirect(impl_cgi_t *this, cad_cgi_response_t *response, const char *redirect, cad_hash_t *extra) {
   assert(!this->ready);
   cad_output_stream_t *body = response->body(response);
   cad_cgi_meta_t *meta = response->meta_variables(response);
   char *resolved_redirect = resolve_string(this, redirect, meta, extra, encode_url);
   assert(resolved_redirect != NULL);
   char *full_redirect;
   if (resolved_redirect[0] == '/') {
      full_redirect = resolved_redirect;
      resolved_redirect = NULL;
   } else {
      full_redirect = szprintf(this->memory, NULL, "%s/%s", meta->script_name(meta), resolved_redirect);
      this->memory.free(resolved_redirect);
   }
   assert(full_redirect != NULL);
   int status;
   cad_cgi_server_protocol_t *protocol = meta->server_protocol(meta);
   if (!!strcmp(protocol->protocol, "HTTP") || (protocol->major == 1 && protocol->minor == 0)) {
      // HTTP/1.0 and non-HTTP protocols
      status = 302;
   } else {
      // HTTP/1.1 and more recent (TODO check HTTP/2.0 rfc)
      status = 303;
   }
   log_debug(this->log, "response redirect: status %d -- %s", status, full_redirect);
   response->set_status(response, status);
   response_security_headers(response);
   response->redirect(response, full_redirect, "");
   body->put(body, "Redirect to %s\n", full_redirect);
   this->ready = 1;
   this->memory.free(full_redirect);
   this->automaton->set_state(this->automaton, State_write_to_client, NULL);
}

void set_response_template(impl_cgi_t *this, cad_cgi_response_t *response, int status, const char *template, cad_hash_t *extra) {
   assert(!this->ready);
   cad_output_stream_t *body = response->body(response);
   cad_cgi_meta_t *meta = response->meta_variables(response);
   cad_array_t *nested = cad_new_array(this->memory, sizeof(cad_hash_t*));
   cad_hash_t *cgi = cad_new_hash(this->memory, cad_hash_strings);

   cgi->set(cgi, "path_info", (void*)meta->path_info(meta));
   cgi->set(cgi, "script_name", (void*)meta->script_name(meta));

   struct meta_data data = {this->log, meta, extra, this->memory, nested, cgi, this->config, NULL};
   char *template_name = resolve_string(this, template, meta, extra, NULL);
   assert(template_name != NULL);
   char *template_path = szprintf(this->memory, NULL, "%s/%s.tpl", this->templates_path, template_name);
   assert(template_path != NULL);
   this->memory.free(template_name);
   int template_fd = open(template_path, O_RDONLY);
   if (template_fd == -1) {
      log_error(this->log, "Error opening template: %s", template_path);
      set_response_string(this, response, 500, "Server error\n");
   } else {
      log_debug(this->log, "status: %d -- template: %s", status, template_path);
      response->set_content_type(response, "text/html"); // TODO config
      response->set_status(response, status);
      cad_input_stream_t *in = new_cad_input_stream_from_file_descriptor(template_fd, this->memory);
      assert(in != NULL);
      cad_stache_t *stache = new_cad_stache(this->memory, (cad_stache_resolve_cb)resolve_meta, &data);
      assert(stache != NULL);
      log_debug(this->log, "Rendering stache");
      stache->render(stache, in, body, template_error, this);
      stache->free(stache);
      in->free(in);
      close(template_fd);
      assert(nested->count(nested) == 0);
      response_security_headers(response);
      this->ready = 1;
      this->automaton->set_state(this->automaton, State_write_to_client, NULL);
   }

   nested->free(nested);
   cgi->free(cgi);
   this->memory.free(template_path);
}
