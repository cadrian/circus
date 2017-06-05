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

#include <sqlite3.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <circus_database.h>

#define FETCH_TO_DO 0
#define FETCH_READY 1
#define FETCH_OFF -1
#define FETCH_ERROR -2

typedef struct database_resultset_sqlite3_s database_resultset_sqlite3_t;
typedef struct database_query_sqlite3_s database_query_sqlite3_t;
typedef struct database_sqlite3_s database_sqlite3_t;

struct database_resultset_sqlite3_s {
   circus_database_resultset_t fn;
   cad_memory_t memory;
   circus_log_t *log;
   char *sql;
   sqlite3_stmt *stmt;
   int fetched;
   database_query_sqlite3_t *query;
};

struct database_query_sqlite3_s {
   circus_database_query_t fn;
   cad_memory_t memory;
   circus_log_t *log;
   sqlite3 *db;
   char *sql;
   sqlite3_stmt *stmt;
   int running;
};

struct database_sqlite3_s {
   circus_database_t fn;
   cad_memory_t memory;
   circus_log_t *log;
   sqlite3 *db;
};

static void ensure_stmt(database_query_sqlite3_t *q) {
   assert(!q->running);
   if (q->stmt == NULL) {
      int n = sqlite3_prepare_v2(q->db, q->sql, -1, &q->stmt, NULL);
      if (n != SQLITE_OK) {
         q->stmt = NULL;
         log_error(q->log, "Error preparing statement: %s -- %s", q->sql, sqlite3_errstr(n));
      }
   }
}

static void requery(database_query_sqlite3_t *q) {
   if (q->stmt != NULL) {
      int n = sqlite3_finalize(q->stmt);
      if (n != SQLITE_OK) {
         log_warning(q->log, "Error in finalize: %s", sqlite3_errstr(n));
      }
   }
   q->stmt = NULL;
   q->running = 0;
}

static int fetch(database_resultset_sqlite3_t *rs) {
   if (rs->fetched == FETCH_TO_DO) {
      switch(sqlite3_step(rs->stmt)) {
      case SQLITE_OK:
      case SQLITE_ROW:
         rs->fetched = FETCH_READY;
         break;
      case SQLITE_DONE:
         rs->fetched = FETCH_OFF;
         break;
      default:
         rs->fetched = FETCH_ERROR;
         break;
      }
   }
   return rs->fetched;
}

static int database_resultset_has_error_sqlite3(database_resultset_sqlite3_t *this) {
   return fetch(this) == FETCH_ERROR;
}

static int database_resultset_has_next_sqlite3(database_resultset_sqlite3_t *this) {
   return fetch(this) == FETCH_READY;
}

static int database_resultset_next_sqlite3(database_resultset_sqlite3_t *this) {
   assert(fetch(this) == FETCH_READY);
   this->fetched = FETCH_TO_DO;
   return 1;
}

static int64_t database_resultset_get_int_sqlite3(database_resultset_sqlite3_t *this, int index) {
   assert(index >= 0 && index < sqlite3_column_count(this->stmt));
   assert(this->fetched == FETCH_TO_DO);
   return (int64_t)sqlite3_column_int64(this->stmt, index);
}

static const char *database_resultset_get_string_sqlite3(database_resultset_sqlite3_t *this, int index) {
   assert(index >= 0 && index < sqlite3_column_count(this->stmt));
   assert(this->fetched == FETCH_TO_DO);
   return (const char*)sqlite3_column_text(this->stmt, index);
}

static void database_resultset_free_sqlite3(database_resultset_sqlite3_t *this) {
   requery(this->query);
   this->memory.free(this);
}

static circus_database_resultset_t database_resultset_sqlite3_fn = {
   (circus_database_resultset_has_error_fn) database_resultset_has_error_sqlite3,
   (circus_database_resultset_has_next_fn) database_resultset_has_next_sqlite3,
   (circus_database_resultset_next_fn) database_resultset_next_sqlite3,
   (circus_database_resultset_get_int_fn) database_resultset_get_int_sqlite3,
   (circus_database_resultset_get_string_fn) database_resultset_get_string_sqlite3,
   (circus_database_resultset_free_fn) database_resultset_free_sqlite3,
};

static int database_query_set_int_sqlite3(database_query_sqlite3_t *this, int index, int64_t value) {
   assert(!this->running);
   ensure_stmt(this);
   assert(index >= 0 && index < sqlite3_bind_parameter_count(this->stmt));
   int n = sqlite3_bind_int64(this->stmt, index + 1, (sqlite3_int64)value);
   if (n != SQLITE_OK) {
      log_error(this->log, "Error binding parameter #%d: %s -- %s", index, this->sql, sqlite3_errstr(n));
      return 0;
   }
   return 1;
}

static int database_query_set_string_sqlite3(database_query_sqlite3_t *this, int index, const char *value) {
   assert(!this->running);
   ensure_stmt(this);
   assert(index >= 0 && index < sqlite3_bind_parameter_count(this->stmt));
   int n = sqlite3_bind_text(this->stmt, index + 1, value, -1, SQLITE_TRANSIENT);
   if (n != SQLITE_OK) {
      log_error(this->log, "Error binding parameter #%d: %s -- %s", index, this->sql, sqlite3_errstr(n));
      return 0;
   }
   return 1;
}

static database_resultset_sqlite3_t *database_query_run_sqlite3(database_query_sqlite3_t *this) {
   assert(!this->running);
   ensure_stmt(this);
   database_resultset_sqlite3_t *result = this->memory.malloc(sizeof(database_resultset_sqlite3_t));
   assert(result != NULL);

   result->fn = database_resultset_sqlite3_fn;
   result->memory = this->memory;
   result->log = this->log;
   result->sql = this->sql;
   result->stmt = this->stmt;
   result->fetched = FETCH_TO_DO;
   result->query = this;

   this->running = 1;
   fetch(result);

   return result;
}

static void database_query_free_sqlite3(database_query_sqlite3_t *this) {
   requery(this);
   int n = sqlite3_db_cacheflush(this->db);
   if (n != SQLITE_OK) {
      log_error(this->log, "Error flushing: %s -- %s", this->sql, sqlite3_errstr(n));
   }
   this->memory.free(this->sql);
   this->memory.free(this);
}

static circus_database_query_t database_query_sqlite3_fn = {
   (circus_database_query_set_int_fn)database_query_set_int_sqlite3,
   (circus_database_query_set_string_fn)database_query_set_string_sqlite3,
   (circus_database_query_run_fn)database_query_run_sqlite3,
   (circus_database_query_free_fn)database_query_free_sqlite3,
};

static database_query_sqlite3_t *database_query_sqlite3(database_sqlite3_t *this, const char *sql) {
   database_query_sqlite3_t *result = NULL;
   result = this->memory.malloc(sizeof(database_query_sqlite3_t));
   assert(result != NULL);
   result->fn = database_query_sqlite3_fn;
   result->memory = this->memory;
   result->log = this->log;
   result->db = this->db;
   result->sql = szprintf(this->memory, NULL, "%s", sql);
   result->stmt = NULL;
   result->running = 0;
   return result;
}

static void database_free_sqlite3(database_sqlite3_t *this) {
   sqlite3_close(this->db);
   this->memory.free(this);
}

static circus_database_t database_sqlite3_fn = {
   (circus_database_query_fn) database_query_sqlite3,
   (circus_database_free_fn) database_free_sqlite3,
};

static void mkparentdirs(cad_memory_t memory, const char *dir) {
   char *tmp;
   char *p = NULL;
   int len, i;

   tmp = szprintf(memory, &len, "%s", dir);
   assert(tmp != NULL);
   if (tmp[len - 1] == '/') {
      tmp[--len] = 0;
   }
   for (i = len - 1; i > 0 && tmp[i] != '/'; i--) {
      // just looping to find the last '/', to remove the filename
   }
   if (i > 0) {
      assert(tmp[i] == '/');
      tmp[i] = 0;

      for (p = tmp + 1; *p; p++) {
         if (*p == '/') {
            *p = 0;
            mkdir(tmp, 0770);
            *p = '/';
         }
      }
      mkdir(tmp, 0700);
   }

   memory.free(tmp);
}

circus_database_t *circus_database_sqlite3(cad_memory_t memory, circus_log_t *log, const char *path) {
   database_sqlite3_t *result = memory.malloc(sizeof(database_sqlite3_t));
   assert(result != NULL);

   mkparentdirs(memory, path);

   result->fn = database_sqlite3_fn;
   result->memory = memory;
   result->log = log;

   int n = sqlite3_open_v2(path, &(result->db),
                           SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_PRIVATECACHE,
                           "unix-excl");
   if (n != SQLITE_OK) {
      log_error(log, "Cannot open database: %s -- %s", path, sqlite3_errmsg(result->db));
      sqlite3_close(result->db);
      memory.free(result);
      return NULL;
   }

   return I(result);
}
