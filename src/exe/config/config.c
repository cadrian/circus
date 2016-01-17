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

#include <cad_hash.h>
#include <cad_shared.h>
#include <cad_stream.h>
#include <errno.h>
#include <json.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "circus.h"
#include "config.h"
#include "log.h"
#include "xdg.h"

extern circus_log_t *LOG;

typedef struct {
   circus_config_t fn;
   cad_memory_t memory;
   json_object_t *data;
   int dirty;
   char *filename;
   int local;
} config_impl;

typedef struct {
   FILE *file;
   char *path;
   int local;
} read_t;

static read_t read_xdg_file_from_config_dirs(cad_memory_t memory, const char *filename) {
   int n = strlen(xdg_config_dirs()) + 1;
   char *config_dirs = memory.malloc(n);
   char *cur = config_dirs, *next=cur;
   read_t result = { NULL, NULL, 0 };
   snprintf(config_dirs, n, "%s", xdg_config_dirs());
   while (result.file == NULL && next != NULL) {
      memory.free(result.path);
      cur = next;
      next = strchr(cur, ':');
      if (next != NULL) {
         *next = '\0';
         next++;
      }
      result.path = szprintf(memory, NULL, "%s/%s", cur, filename);
      result.file = fopen(result.path, "r");
   }
   if (result.file == NULL) {
      memory.free(result.path);
   }
   memory.free(config_dirs);
   return result;
}

static read_t read_xdg_file_dirs(cad_memory_t memory, const char *filename) {
   read_t result = { NULL, NULL, 1 };
   result.path = szprintf(memory, NULL, "%s/%s", xdg_data_home(), filename);
   result.file = fopen(result.path, "r");
   if (result.file == NULL) {
      memory.free(result.path);
      result = read_xdg_file_from_config_dirs(memory, filename);
   }
   return result;
}

static const char *config_get(config_impl *this, const char *section, const char *key) {
   json_string_t *result = (json_string_t*)json_lookup((json_value_t*)this->data, section, key, JSON_STOP);
   static char *buffer = NULL;
   static size_t buflen = 0;
   if (result != NULL) {
      size_t n = result->utf8(result, buffer, buflen);
      if (n >= buflen) {
         buflen = n + 1;
         buffer = this->memory.realloc(buffer, buflen);
         n = result->utf8(result, buffer, buflen);
         assert(n = buflen - 1);
      }
   }
   return (const char *)buffer;
}

static void config_set(config_impl *this, const char *section, const char *key, const char *value) {
   json_string_t *string = (json_string_t*)json_lookup((json_value_t*)this->data, section, key, JSON_STOP);
   json_object_t *section_object = (json_object_t*)json_lookup((json_value_t*)this->data, section, JSON_STOP);
   if (string == NULL) {
      if (section_object == NULL) {
         section_object = json_new_object(this->memory);
         this->data->set(this->data, section, (json_value_t*)section_object);
      }
   } else {
      string->free(string);
      assert(section_object != NULL);
   }
   string = json_new_string(this->memory);
   section_object->set(section_object, key, (json_value_t*)string);
   string->add_string(string, "%s", value);
   this->dirty = 1;
}

static void config_backup(config_impl *this) {
   int n = strlen(this->filename) + 2;
   char *backup_filename = this->memory.malloc(n);
   snprintf(backup_filename, n, "%s~", this->filename);
   n = rename(this->filename, backup_filename);
   if (n < 0) {
      if (errno != ENOENT) {
         perror("rename backup config file");
         exit(1);
      }
   }
   this->memory.free(backup_filename);
}

static void config_write(config_impl *this) {
   FILE *file;
   char *path;
   cad_output_stream_t *stream;
   json_visitor_t *writer;
   int n;

   if (this->dirty) {
      config_backup(this);

      path = szprintf(this->memory, NULL, "%s/%s", xdg_data_home(), this->filename);
      file = fopen(path, "w");
      if (file == NULL) {
         log_error(LOG, "config", "Could not write config file: %s", path);
         perror("fopen write config file");
         exit(1);
      }
      stream = new_cad_output_stream_from_file(file, this->memory);
      writer = json_write_to(stream, this->memory, json_extend_spaces);

      this->data->accept(this->data, writer);
      stream->flush(stream);

      writer->free(writer);
      stream->free(stream);
      n = fclose(file);
      assert(n == 0);
      this->memory.free(path);

      this->dirty = 0;
      this->local = 1;
   }
}

static void config_free(config_impl *this) {
   this->memory.free(this);
}

static circus_config_t impl_fn = {
   (circus_config_get_fn)config_get,
   (circus_config_set_fn)config_set,
   (circus_config_write_fn)config_write,
   (circus_config_free_fn)config_free,
};

static void config_error(cad_input_stream_t *stream, int line, int column, void *data, const char *format, ...) {
   UNUSED(stream);
   config_impl *this = (config_impl*)data;
   va_list args;
   char *log;
   va_start(args, format);
   log = vszprintf(this->memory, NULL, format, args);
   va_end(args);
   log_error(LOG, "config", "Error while reading config file, line %d, column %d: %s", line, column, log);
   this->memory.free(log);
}

typedef struct {
   json_visitor_t fn;
   int depth;
} config_read_checker;

static void config_read_checker_free(config_read_checker *this) {
   // never allocated
   UNUSED(this);
}

static void config_read_checker_visit_object(config_read_checker *this, json_object_t *visited) {
   int i;
   int n = visited->count(visited);
   const char **keys;
   if (this->depth > 1) {
      fprintf(stderr, "**** Invalid config, object found at depth %d", this->depth);
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
}

static void config_read_checker_visit_array(config_read_checker *this, json_array_t *visited) {
   UNUSED(visited);
   fprintf(stderr, "**** Invalid config, array found at depth %d", this->depth);
   exit(1);
}

static void config_read_checker_visit_string(config_read_checker *this, json_string_t *visited) {
   UNUSED(visited);
   if (this->depth != 2) {
      fprintf(stderr, "**** Invalid config, string found at depth %d", this->depth);
      exit(1);
   }
}

static void config_read_checker_visit_number(config_read_checker *this, json_number_t *visited) {
   UNUSED(visited);
   fprintf(stderr, "**** Invalid config, number found at depth %d", this->depth);
   exit(1);
}

static void config_read_checker_visit_const(config_read_checker *this, json_const_t *visited) {
   UNUSED(visited);
   fprintf(stderr, "**** Invalid config, const found at depth %d", this->depth);
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
   int n = strlen(filename) + 1;
   config_impl *result;
   read_t read;
   cad_input_stream_t *raw, *stream;
   json_value_t *data;

   result = memory.malloc(sizeof(config_impl) + n);
   assert(result != NULL);

   result->fn = impl_fn;
   result->memory = memory;
   result->filename = (char*)(result + 1);
   strncpy(result->filename, filename, n);

   read = read_xdg_file_dirs(memory, filename);
   if (read.file == NULL) {
      data = (json_value_t*)json_new_object(memory);
      result->dirty = 1;
      result->local = 0;
   } else {
      result->local = read.local;
      raw = new_cad_input_stream_from_file(read.file, memory);
      assert(raw != NULL);
      stream = new_json_utf8_stream(raw, memory);
      assert(stream != NULL);

      data = json_parse(stream, config_error, result, memory);
      if (data == NULL) {
         log_error(LOG, "config", "JSON parse error in file %s", read.path);
         exit(1);
      }

      config_read_checker checker = { config_read_checker_fn, 0 };
      data->accept(data, (json_visitor_t*)&checker);

      stream->free(stream);
      raw->free(raw);
      n = fclose(read.file);
      assert(n == 0);

      memory.free(read.path);
   }
   result->data = (json_object_t *)data;

   return (circus_config_t*)result;
}
