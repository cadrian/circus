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

#include <circus.h>
#include <circus_log.h>
#include <circus_vault.h>

#include "vault_impl.h"
#include "vault_pass.h"

uint64_t get_stretch_threshold(circus_log_t *log, circus_database_t *database) {
   static const char *sql = "SELECT VALUE FROM META WHERE KEY=?";
   circus_database_query_t *q = database->query(database, sql);
   int ok;

   uint64_t result;

   if (q != NULL) {
      ok = q->set_string(q, 0, "STRETCH");
      if (ok) {
         circus_database_resultset_t *rs = q->run(q);
         if (rs != NULL) {
            if (!rs->has_next(rs)) {
               result = DEFAULT_STRETCH;
               log_warning(log, "Consider defining STRETCH in META. Using default %"PRIu64, result);
            } else {
               rs->next(rs);
               result = (uint64_t)rs->get_int(rs, 0);
               if (result < DEFAULT_STRETCH) {
                  log_warning(log, "Consider defining stronger STRETCH in META. %"PRIu64" is not enough (consider at least %"PRIu64")",
                              result, DEFAULT_STRETCH);
               }
            }
            rs->free(rs);
         }
      }
      q->free(q);
   }

   return result;
}

int set_stretch_threshold(circus_log_t *log, circus_database_t *database, uint64_t stretch_threshold) {
   static const char *sql = "INSERT OR REPLACE INTO META (KEY, VALUE) VALUES (?, ?)";
   circus_database_query_t *q = database->query(database, sql);
   int ok;

   if (stretch_threshold < DEFAULT_STRETCH) {
      log_warning(log, "Selected stretch threshold %"PRIu64" is too low; setting to %"PRIu64" instead", stretch_threshold, DEFAULT_STRETCH);
      stretch_threshold = DEFAULT_STRETCH;
   }

   if (q == NULL) {
      ok = 0;
   } else {
      ok = q->set_string(q, 0, "STRETCH");
      if (ok) {
         ok = q->set_int(q, 1, (int64_t)stretch_threshold);
         if (ok) {
            circus_database_resultset_t *rs = q->run(q);
            if (rs == NULL) {
               ok = 0;
            } else {
               if (rs->has_error(rs)) {
                  log_warning(log, "Could not set stretch threshold");
                  ok = 0;
               }
               rs->free(rs);
            }
         }
      }
      q->free(q);
   }

   return ok;
}
