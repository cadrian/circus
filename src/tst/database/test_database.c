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

#include <inttypes.h>
#include <string.h>

#include "_test_database.h"

#include <circus_database.h>

circus_log_t *LOG;

static char* data[] = { "foo", "bar", NULL };

static int check_data(sqlite3_stmt *stmt) {
   static int id = 0;
   assert(!strcmp((char*)sqlite3_column_name(stmt, 0), "ID"));
   assert(!strcmp((char*)sqlite3_column_name(stmt, 1), "VALUE"));
   assert(sqlite3_column_int64(stmt, 0) == (sqlite3_int64)id + 1);
   assert(!strcmp((char*)sqlite3_column_text(stmt, 1), data[id]));
   id++;
   return 1;
}

int main() {
   LOG = circus_new_log_file_descriptor(stdlib_memory, LOG_PII, 1);
   const char *path = "test_database.db";
   circus_database_query_t *q;
   circus_database_resultset_t *rs;
   int i;

   circus_database_t *db = circus_database_sqlite3(stdlib_memory, LOG, path);
   assert(db != NULL);

   q = db->query(db, "CREATE TABLE IF NOT EXISTS TEST(ID INTEGER PRIMARY KEY AUTOINCREMENT, VALUE TEXT);");
   rs = q->run(q);
   assert(!rs->has_next(rs));
   rs->free(rs);
   q->free(q);

   q = db->query(db, "INSERT INTO TEST (VALUE) VALUES (?);");
   for (i = 0; data[i] != NULL; i++) {
      assert(q->set_string(q, 0, data[i]));
      rs = q->run(q);
      assert(!rs->has_next(rs));
      rs->free(rs);
   }
   q->free(q);

   q = db->query(db, "SELECT ID, VALUE FROM TEST ORDER BY ID;");
   rs = q->run(q);
   for (i = 0; data[i] != NULL; i++) {
      assert(rs->has_next(rs));
      rs->next(rs);
      assert((int64_t)i + 1 == rs->get_int(rs, 0));
      assert(!strcmp(data[i], rs->get_string(rs, 1)));
   }
   assert(!rs->has_next(rs));
   rs->free(rs);
   q->free(q);

   db->free(db);

   query_database(path, "SELECT * FROM TEST;", check_data);

   LOG->free(LOG);
   return 0;
}
