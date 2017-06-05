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

#ifndef __CIRCUS_DATABASE_H
#define __CIRCUS_DATABASE_H

#include <inttypes.h>

#include <circus.h>
#include <circus_config.h>
#include <circus_log.h>

typedef struct circus_database_s circus_database_t;
typedef struct circus_database_query_s circus_database_query_t;
typedef struct circus_database_resultset_s circus_database_resultset_t;

typedef circus_database_query_t *(*circus_database_query_fn)(circus_database_t *this, const char *sql);
typedef void (*circus_database_free_fn)(circus_database_t *this);

struct circus_database_s {
   circus_database_query_fn query;
   circus_database_free_fn free;
};

typedef int (*circus_database_query_set_int_fn)(circus_database_query_t *this, int index, int64_t value);
typedef int (*circus_database_query_set_string_fn)(circus_database_query_t *this, int index, const char *value);
typedef circus_database_resultset_t *(*circus_database_query_run_fn)(circus_database_query_t *this);
typedef void (*circus_database_query_free_fn)(circus_database_query_t *this);

struct circus_database_query_s {
   circus_database_query_set_int_fn set_int;
   circus_database_query_set_string_fn set_string;
   circus_database_query_run_fn run;
   circus_database_query_free_fn free;
};

typedef int (*circus_database_resultset_has_error_fn)(circus_database_resultset_t *this);
typedef int (*circus_database_resultset_has_next_fn)(circus_database_resultset_t *this);
typedef int (*circus_database_resultset_next_fn)(circus_database_resultset_t *this);
typedef int64_t (*circus_database_resultset_get_int_fn)(circus_database_resultset_t *this, int index);
typedef const char *(*circus_database_resultset_get_string_fn)(circus_database_resultset_t *this, int index);
typedef void (*circus_database_resultset_free_fn)(circus_database_resultset_t *this);

struct circus_database_resultset_s {
   circus_database_resultset_has_error_fn has_error;
   circus_database_resultset_has_next_fn has_next;
   circus_database_resultset_next_fn next;
   circus_database_resultset_get_int_fn get_int;
   circus_database_resultset_get_string_fn get_string;
   circus_database_resultset_free_fn free;
};

__PUBLIC__ circus_database_t *circus_database_sqlite3(cad_memory_t memory, circus_log_t *log, const char *path);
__PUBLIC__ int database_exec(circus_log_t *log, circus_database_t *database, const char *sql);

#endif /* __CIRCUS_DATABASE_H */
