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

#include <cad_shared.h>
#include <stdio.h>
#include "_test_database.h"

void query_database(const char *path, const char *query, database_fn fn) {
   sqlite3 *db;
   int n = sqlite3_open_v2(path, &db,
                           SQLITE_OPEN_READONLY | SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_PRIVATECACHE,
                           "unix-excl");
   if (n != SQLITE_OK) {
      printf("error opening database: %s\n", sqlite3_errstr(n));
      return;
   }
   sqlite3_stmt *stmt;
   n = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
   if (n != SQLITE_OK) {
      printf("error preparing database statement: %s\n", sqlite3_errstr(n));
      return;
   }

   int done = 0;
   do {
      n = sqlite3_step(stmt);
      switch(n) {
      case SQLITE_OK:
      case SQLITE_ROW:
         done = !fn(stmt);
         break;
      case SQLITE_DONE:
         done = 1;
         break;
      default:
         printf("database error: %s\n", sqlite3_errstr(n));
         done = 1;
      }
   } while (!done);

   sqlite3_finalize(stmt);
   sqlite3_close(db);
}
