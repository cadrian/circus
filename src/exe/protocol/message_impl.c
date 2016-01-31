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

#include <cad_array.h>
#include <cad_shared.h>
#include <string.h>
#include <circus.h>
#include <circus_message_impl.h>

static char *dup_string(cad_memory_t memory, const char *string) {
   int n = strlen(string) + 1;
   char *result = memory.malloc(n);
   assert(result != NULL);
   snprintf(result, n, "%s", string);
   return result;
}

static cad_array_t *dup_strings(cad_memory_t memory, cad_array_t *strings) {
   cad_array_t *result = cad_new_array(memory, sizeof(char*));
   int i, n = strings->count(strings);
   char *item;
   assert(result != NULL);
   for (i = 0; i < n; i++) {
      item = strings->get(strings, i);
      result->insert(result, i, dup_string(memory, item));
   }
   return result;
}

static int dup_boolean(cad_memory_t UNUSED(memory), int boolean) {
   return boolean;
}

static char *json_string(cad_memory_t memory, json_string_t *string) {
   int n = string->utf8(string, "", 0) + 1;
   char *result = memory.malloc(n);
   assert(result != NULL);
   string->utf8(string, result, n);
   return result;
}

static cad_array_t *json_strings(cad_memory_t memory, json_array_t *array) {
   cad_array_t *result = cad_new_array(memory, sizeof(char*));
   int i, n = array->count(array);
   json_string_t *string;
   char *item;
   assert(result != NULL);
   for (i = 0; i < n; i++) {
      string = (json_string_t*)array->get(array, i);
      item = json_string(memory, string);
      result->insert(result, i, item);
   }
   return result;
}

static int json_boolean(json_const_t *cons) {
   return cons->value(cons);
}

#include "gen/factory.c"
