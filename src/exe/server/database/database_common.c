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

#include <circus_database.h>

int database_exec(circus_log_t *log, circus_database_t *database, const char *sql) {
   circus_database_query_t *q;
   int result;
   q = database->query(database, sql);
   if (q == NULL) {
      result = 0;
   } else {
      circus_database_resultset_t *rs = q->run(q);
      if (rs == NULL) {
         result = 0;
      } else {
         while (rs->has_next(rs)) {
            rs->next(rs);
         }
         result = !rs->has_error(rs);
         rs->free(rs);
      }
      q->free(q);
   }
   if (!result) {
      log_error(log, "Error executing: %s", sql);
   }
   return result;
}
