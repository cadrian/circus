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

#include <string.h>

#include <cad_array.h>
#include <circus_crypt.h>
#include <circus_password.h>

/*
 * RECIPE GRAMMAR:
 *
 * Recipe          ::= Mix+                                    # at least one mix
 * Mix             ::= Quantities Ingredient
 * Quantities      ::= Quantity                                # fixed quantity
 *                   | Quantity "-" Quantity                   # quantity in random range
 * Quantity        ::= "[0-9]*"
 * Ingredient      ::= StandardSets
 *                   | FreeIngredients
 * StandardSets    ::= StandardSet+
 * StandardSet     ::= "[Aa]"                                  # alphabetic
 *                   | "[Nn]"                                  # numeric
 *                   | "[Ss]"                                  # symbols
 * FreeIngredients ::= "["]([^"]|["].)["]|[']([^']|['].)[']"
 */

typedef struct pass_generator_mix_s pass_generator_mix_t;
struct pass_generator_mix_s {
   unsigned int min_quantity;
   unsigned int max_quantity;
   char *ingredient;
};

typedef struct pass_generator_s pass_generator_t;
struct pass_generator_s {
   cad_array_t *mixes;
};

typedef struct buffer_s buffer_t;
struct buffer_s {
   const char *string;
   unsigned int index;
   unsigned int size;
   const char *error;

   unsigned int last_quantity;
   char *last_ingredient;
};

static char *FIGURES = "0123456789";
static char *LETTERS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
static char *SYMBOLS = "(-_)~#{[|^@]}+=<>,?./!";

static void free_generator(cad_memory_t memory, pass_generator_t *generator) {
   unsigned int i, n = generator->mixes->count(generator->mixes);
   for (i = 0; i < n; i++) {
      pass_generator_mix_t *mix = generator->mixes->get(generator->mixes, i);
      memory.free(mix->ingredient);
   }
   generator->mixes->clear(generator->mixes);
   generator->mixes->free(generator->mixes);
   memory.free(generator);
}

static void skip_blanks(buffer_t *buffer) {
   int ok = 1;
   char c;
   while (ok && buffer->index < buffer->size) {
      c = buffer->string[buffer->index];
      switch (c) {
      case ' ': case '\r': case '\n':
         buffer->index++;
         break;
      default:
         ok = 0;
      }
   }
}

static void parse_quantity(buffer_t *buffer) {
   unsigned int last_quantity = 0;
   int ok = 1;
   char c;
   skip_blanks(buffer);
   while (ok && buffer->index < buffer->size) {
      c = buffer->string[buffer->index];
      switch (c) {
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
         last_quantity = last_quantity * 10 + c - '0';
         buffer->index++;
         break;
      default:
         ok = 0;
      }
   }
   buffer->last_quantity = last_quantity == 0 ? 1 : last_quantity;
}

static void extend(cad_memory_t memory, char *string, char c, int l, int *n) {
   if (l == *n) {
      int m = *n * 2;
      string = memory.realloc(string, m);
      *n = m;
   }
   string[l] = c;
}

static char *parse_string(cad_memory_t memory, buffer_t *buffer) {
   char delim = buffer->string[buffer->index++];
   int s = 1, l = 0, n = 16;
   char c;
   char *result = memory.malloc(n);
   while (s > 0) {
      if (buffer->index < buffer->size) {
         c = buffer->string[buffer->index];
         switch (s) {
         case 1:
            if (c == delim) {
               s = 0;
            } else if (c == '\\') {
               s = 1;
            } else {
               extend(memory, result, c, l++, &n);
            }
            break;
         case 2:
            extend(memory, result, c, l++, &n);
            break;
         }
         buffer->index++;
      } else {
         buffer->error = "Unterminated string";
         s = 0;
      }
   }
   extend(memory, result, '\0', l, &n);
   return result;
}

static void parse_more_ingredient(cad_memory_t memory, buffer_t *buffer) {
   char c;
   skip_blanks(buffer);
   char* ingredient = buffer->last_ingredient, *new_ingredient;
   int ok = 1;
   while (ok && buffer->index < buffer->size) {
      c = buffer->string[buffer->index];
      switch (c) {
      case 'a': case 'A':
         new_ingredient = szprintf(memory, NULL, "%s%s", ingredient, LETTERS);
         break;
      case 'n': case 'N':
         new_ingredient = szprintf(memory, NULL, "%s%s", ingredient, FIGURES);
         break;
      case 's': case 'S':
         new_ingredient = szprintf(memory, NULL, "%s%s", ingredient, SYMBOLS);
         break;
      default:
         ok = 0;
      }
      if (ok) {
         memory.free(ingredient);
         ingredient = new_ingredient;
         buffer->index++;
      }
   }
   buffer->last_ingredient = ingredient;
}

static void parse_ingredient(cad_memory_t memory, buffer_t *buffer) {
   char c;
   skip_blanks(buffer);
   if (buffer->index >= buffer->size) {
      buffer->error = "Expecting ingredient specification";
   } else {
      c = buffer->string[buffer->index];
      switch (c) {
      case 'a': case 'A':
         buffer->last_ingredient = szprintf(memory, NULL, "%s", LETTERS);
         buffer->index++;
         parse_more_ingredient(memory, buffer);
         break;
      case 'n': case 'N':
         buffer->last_ingredient = szprintf(memory, NULL, "%s", FIGURES);
         buffer->index++;
         parse_more_ingredient(memory, buffer);
         break;
      case 's': case 'S':
         buffer->last_ingredient = szprintf(memory, NULL, "%s", SYMBOLS);
         buffer->index++;
         parse_more_ingredient(memory, buffer);
         break;
      case '"': case '\'':
         buffer->last_ingredient = parse_string(memory, buffer);
         break;
      default:
         buffer->error = "Invalid ingredient specification";
      }
   }
}

static void parse_mix(cad_memory_t memory, circus_log_t *log, buffer_t *buffer, pass_generator_t *generator) {
   unsigned int min_quantity, max_quantity;
   parse_quantity(buffer);
   if (!buffer->error && buffer->index < buffer->size) {
      min_quantity = buffer->last_quantity;
      skip_blanks(buffer);
      if (!buffer->error && buffer->index < buffer->size) {
         if (buffer->string[buffer->index] == '-') {
            buffer->index++;
            skip_blanks(buffer);
            parse_quantity(buffer);
            max_quantity = buffer->last_quantity;
            if (max_quantity < min_quantity) {
               buffer->error = "Invalid quantity range: min > max";
            }
         } else {
            max_quantity = min_quantity;
         }
         if (!buffer->error && buffer->index < buffer->size) {
            parse_ingredient(memory, buffer);
            if (!buffer->error) {
               log_debug(log, "mix: %u-%u / %s", min_quantity, max_quantity, buffer->last_ingredient);
               pass_generator_mix_t mix = {min_quantity, max_quantity, buffer->last_ingredient};
               generator->mixes->insert(generator->mixes, generator->mixes->count(generator->mixes), &mix);
            }
         }
      } else {
         buffer->error = "Expecting ingredient specification";
      }
   } else {
      buffer->error = "Expecting ingredient specification";
   }
}

static pass_generator_t *parse_recipe(cad_memory_t memory, circus_log_t *log, const char *recipe, char **error) {
   pass_generator_t *result = memory.malloc(sizeof(pass_generator_t));
   if (result != NULL) {
      result->mixes = cad_new_array(memory, sizeof(pass_generator_mix_t));
      buffer_t buffer = {recipe, 0, strlen(recipe), NULL, 0, NULL};
      while (buffer.index < buffer.size && !buffer.error) {
         parse_mix(memory, log, &buffer, result);
      }
      if (buffer.error) {
         *error = szprintf(memory, NULL, "%d: %s", buffer.index + 1, buffer.error);
         log_error(log, "Invalid recipe [%s]: %s", recipe, *error);
         free_generator(memory, result);
         result = NULL;
      } else if (result->mixes->count(result->mixes) == 0) {
         *error = szprintf(memory, NULL, "0: empty recipe");
         log_error(log, "Invalid recipe [%s]: %s", recipe, *error);
         free_generator(memory, result);
         result = NULL;
      }
   }
   return result;
}

char *generate_pass(cad_memory_t memory, circus_log_t *log, const char *recipe, char **error) {
   pass_generator_t *generator = parse_recipe(memory, log, recipe, error);
   if (generator == NULL) {
      return NULL;
   }
   unsigned int passlen = 0, minlen = 0, maxlen = 0, n = generator->mixes->count(generator->mixes), i, j, l, p, index = 0;
   unsigned int *mixlens = memory.malloc(n * sizeof(unsigned int));
   char c;

   for (i = 0; i < n; i++) {
      pass_generator_mix_t *mix = generator->mixes->get(generator->mixes, i);
      unsigned int delta = mix->max_quantity - mix->min_quantity;
      minlen += mix->min_quantity;
      maxlen += mix->max_quantity;
      if (delta == 0) {
         mixlens[i] = mix->max_quantity;
      } else {
         mixlens[i] = mix->min_quantity + irandom(mix->max_quantity - mix->min_quantity);
      }
      passlen += mixlens[i];
   }
   log_debug(log, "Generator passlen[%d-%d]=%d, mix count=%d", minlen, maxlen, passlen, n);

   char *result = memory.malloc(passlen + 1);
   if (result != NULL) {
      for (i = 0; i < n; i++) {
         pass_generator_mix_t *mix = generator->mixes->get(generator->mixes, i);
         l = strlen(mix->ingredient);
         for (j = 0; j < mixlens[i]; j++) {
            c = mix->ingredient[irandom(l)];
            index++;
            p = irandom(index);
            if (p < index - 1) {
               memmove(result + p + 1, result + p, index - p);
            }
            result[p] = c;
         }
      }
      result[passlen] = '\0';
      log_pii(log, "Generated a new password of length %d: %s", passlen, result);
   } else {
      log_debug(log, "Could not generate password");
   }

   memory.free(mixlens);
   free_generator(memory, generator);
   return result;
}
