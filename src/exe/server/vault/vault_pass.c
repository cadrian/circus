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

#include <circus.h>
#include <circus_crypt.h>

#include "vault_pass.h"

static int pass_stretch(cad_memory_t memory, circus_log_t *log, hashing_t *hashing) {
   assert(hashing != NULL);
   assert(hashing->stretch >= DEFAULT_STRETCH);
   assert(hashing->clear != NULL && hashing->clear[0] != '\0');
   assert(hashing->salt != NULL && hashing->salt[0] != '\0');
   assert(hashing->hashed == NULL);

   log_debug(log, "stretch=%"PRIu64, hashing->stretch);

   int result;

   char *salt = szprintf(memory, NULL, "%s", hashing->salt);
   if (salt == NULL) {
      log_error(log, "could not allocate memory for salt");
      result = 0;
   } else {
      result = 1;
      char *acc = hashing->clear;
      for (uint64_t i = 0; result && i < hashing->stretch; i++) {
         char *dec = salted(memory, log, salt, hashing->clear);
         if (dec == NULL) {
            log_error(log, "could not salt (%"PRIu64" / %"PRIu64, i, hashing->stretch);
            result = 0;
         } else {
            char *enc = hashed(memory, log, dec);
            memory.free(dec);
            if (enc == NULL) {
               log_error(log, "could not hash (%"PRIu64" / %"PRIu64, i, hashing->stretch);
               result = 0;
            } else {
               memory.free(salt);
               acc = salt = enc;
            }
         }
      }
      if (result) {
         hashing->hashed = acc;
      }
   }

   return result;
}

int pass_hash(cad_memory_t memory, circus_log_t *log, hashing_t *hashing) {
   assert(hashing != NULL);
   assert(hashing->clear != NULL && hashing->clear[0] != '\0');
   assert(hashing->salt == NULL);
   assert(hashing->hashed == NULL);

   log_debug(log, "stretch=%"PRIu64, hashing->stretch);

   int result;

   if (hashing->stretch < DEFAULT_STRETCH) {
      log_warning(log, "Using default stretch: %"PRIu64" instead of provided (weak) %"PRIu64, DEFAULT_STRETCH, hashing->stretch);
      hashing->stretch = DEFAULT_STRETCH;
   }

   hashing->salt = salt(memory, log);
   if (hashing->salt == NULL) {
      log_error(log, "Could not get salt");
      result = 0;
   } else {
      result = pass_stretch(memory, log, hashing);
      if (!result) {
         memory.free(hashing->salt);
         hashing->salt = NULL;
      }
   }

   return result;
}

int pass_compare(cad_memory_t memory, circus_log_t *log, hashing_t *hashing, uint64_t min_stretch) {
   assert(hashing != NULL);
   assert(hashing->clear != NULL && hashing->clear[0] != '\0');
   assert(hashing->salt != NULL && hashing->salt[0] != '\0');
   assert(hashing->hashed != NULL && hashing->hashed[0] != '\0');

   log_debug(log, "stretch=%"PRIu64, hashing->stretch);

   int result;

   hashing_t cmp;
   cmp.stretch = hashing->stretch;
   cmp.salt = hashing->salt;
   cmp.clear = hashing->clear;
   cmp.hashed = NULL;

   result = pass_stretch(memory, log, &cmp);
   if (result) {
      if (strcmp(cmp.hashed, hashing->hashed)) {
         result = 0;
      } else {
         result = 1;
         if (min_stretch > hashing->stretch) {
            log_debug(log, "re-stretching to %"PRIu64, min_stretch);
            hashing->stretch = min_stretch;
            result = pass_stretch(memory, log, hashing);
         }
      }
   }

   return result;
}
