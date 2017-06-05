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

#include <cad_hash.h>
#include <cad_shared.h>
#include <cad_stream.h>
#include <errno.h>
#include <json.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <circus_config.h>
#include <circus_xdg.h>

typedef struct {
   circus_config_t fn;
   cad_memory_t memory;
   json_object_t *data;
   char *path;
} config_impl;

static const char *config_get(config_impl *this, const char *section, const char *key) {
   json_string_t *entry = (json_string_t*)json_lookup((json_value_t*)this->data, section, key, JSON_STOP);
   const char *result = NULL;
   static char *buffer = NULL;
   static size_t buflen = 0;
   if (entry != NULL) {
      size_t n = entry->utf8(entry, buffer, buflen);
      if (n >= buflen) {
         buflen = n + 1;
         buffer = this->memory.realloc(buffer, buflen);
         n = entry->utf8(entry, buffer, buflen);
         assert(n = buflen - 1);
      }
      result = buffer;
   }
   return result;
}

static const char *config_path(config_impl *this) {
   return this->path;
}

static void config_free(config_impl *this) {
   this->data->accept(this->data, json_kill());
   this->memory.free(this->path);
   this->memory.free(this);
}

static circus_config_t impl_fn = {
   (circus_config_get_fn)config_get,
   (circus_config_path_fn)config_path,
   (circus_config_free_fn)config_free,
};

static void config_error(cad_input_stream_t *UNUSED(stream), int line, int column, void *data, const char *format, ...) {
   config_impl *this = (config_impl*)data;
   va_list args;
   char *log;
   va_start(args, format);
   log = vszprintf(this->memory, NULL, format, args);
   va_end(args);
   fprintf(stderr, "Error while reading config file, line %d, column %d: %s\n", line, column, log);
   this->memory.free(log);
}

typedef struct {
   json_visitor_t fn;
   int depth;
} config_read_checker;

static void config_read_checker_free(config_read_checker *UNUSED(this)) {
   // never allocated
}

static void config_read_checker_visit_object(config_read_checker *this, json_object_t *visited) {
   SET_CANARY();

   int i;
   int n = visited->count(visited);
   const char **keys;
   if (this->depth > 1) {
      fprintf(stderr, "**** Invalid config, object found at depth %d\n", this->depth);
      exit(1);
   }

   keys = alloca(n * sizeof(const char*));
   visited->keys(visited, keys);
   this->depth++;
   for (i = 0; i < n; i++) {
      json_value_t *v = visited->get(visited, keys[i]);
      v->accept(v, (json_visitor_t*)this);
   }
   this->depth--;

   CHECK_CANARY();
}

static void config_read_checker_visit_array(config_read_checker *this, json_array_t *UNUSED(visited)) {
   fprintf(stderr, "**** Invalid config, array found at depth %d\n", this->depth);
   exit(1);
}

static void config_read_checker_visit_string(config_read_checker *this, json_string_t *UNUSED(visited)) {
   if (this->depth != 2) {
      fprintf(stderr, "**** Invalid config, string found at depth %d\n", this->depth);
      exit(1);
   }
}

static void config_read_checker_visit_number(config_read_checker *this, json_number_t *UNUSED(visited)) {
   fprintf(stderr, "**** Invalid config, number found at depth %d\n", this->depth);
   exit(1);
}

static void config_read_checker_visit_const(config_read_checker *this, json_const_t *UNUSED(visited)) {
   fprintf(stderr, "**** Invalid config, const found at depth %d\n", this->depth);
   exit(1);
}

static json_visitor_t config_read_checker_fn = {
   (json_visit_free_fn)config_read_checker_free,
   (json_visit_object_fn)config_read_checker_visit_object,
   (json_visit_array_fn)config_read_checker_visit_array,
   (json_visit_string_fn)config_read_checker_visit_string,
   (json_visit_number_fn)config_read_checker_visit_number,
   (json_visit_const_fn)config_read_checker_visit_const,
};

circus_config_t *circus_config_read(cad_memory_t memory, const char *filename) {
   config_impl *result;
   read_t read;
   cad_input_stream_t *raw, *stream;
   json_value_t *data;
   int n;

   result = memory.malloc(sizeof(config_impl));
   assert(result != NULL);

   result->fn = impl_fn;
   result->memory = memory;

   read = read_xdg_file_from_dirs(memory, filename, xdg_config_dirs());
   if (read.file == NULL) {
      memory.free(read.path);
      data = (json_value_t*)json_new_object(memory);
      result->path = NULL;
   } else {
      raw = new_cad_input_stream_from_file(read.file, memory);
      assert(raw != NULL);
      stream = new_json_utf8_stream(raw, memory);
      assert(stream != NULL);

      data = json_parse(stream, config_error, result, memory);
      if (data == NULL) {
         fprintf(stderr, "JSON parse error in file %s\n", read.path);
         exit(1);
      }

      config_read_checker checker = { config_read_checker_fn, 0 };
      data->accept(data, (json_visitor_t*)&checker);

      stream->free(stream);
      raw->free(raw);
      n = fclose(read.file);
      assert(n == 0);

      result->path = read.path;
   }
   result->data = (json_object_t *)data;

   return I(result);
}
