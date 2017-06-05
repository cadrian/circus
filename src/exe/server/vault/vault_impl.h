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

    Copyright © 2015-2016 Cyril Adrian <cyril.adrian@gmail.com>
*/

#ifndef __CIRCUS_VAULT_IMPL_H
#define __CIRCUS_VAULT_IMPL_H

#include <cad_hash.h>
#include <inttypes.h>

/*
 * Because of how the exe resolver works, it is mandatory that the
 * following inclusion is performed before including this file:
 *
 * #include <circus_vault.h>
 */

#define DB_VERSION "1"

#define PERMISSION_REVOKED  0
#define PERMISSION_USER     1
#define PERMISSION_ADMIN    2

#define DEFAULT_STRETCH ((uint64_t)65536)

#define META_SCHEMA                                          \
   "CREATE TABLE IF NOT EXISTS META (\n"                     \
   "  KEY           TEXT PRIMARY KEY,\n"                     \
   "  VALUE         TEXT NOT NULL\n"                         \
   ");"

#define USERS_SCHEMA                                         \
   "CREATE TABLE IF NOT EXISTS USERS (\n"                    \
   "  USERID        INTEGER PRIMARY KEY AUTOINCREMENT,\n"    \
   "  USERNAME      TEXT NOT NULL,\n"                        \
   "  EMAIL         TEXT,\n"                                 \
   "  PERMISSIONS   INTEGER NOT NULL,\n"                     \
   "  STRETCH       INTEGER NOT NULL,\n"                     \
   "  PWDSALT       TEXT NOT NULL,\n"                        \
   "  HASHPWD       TEXT NOT NULL,\n"                        \
   "  PWDVALID      INTEGER,\n"                              \
   "  KEYSALT       TEXT NOT NULL,\n"                        \
   "  HASHKEY       TEST NOT NULL\n"                         \
   ");\n"                                                    \
   "CREATE UNIQUE INDEX IF NOT EXISTS USERS_IX ON USERS (\n" \
   "  USERNAME\n"                                            \
   ");"

#define KEYS_SCHEMA                                          \
   "CREATE TABLE IF NOT EXISTS KEYS (\n"                     \
   "  KEYID         INTEGER PRIMARY KEY AUTOINCREMENT,\n"    \
   "  USERID        INTEGER NOT NULL,\n"                     \
   "  KEYNAME       TEXT NOT NULL,\n"                        \
   "  SALT          TEXT NOT NULL,\n"                        \
   "  STRETCH       INTEGER NOT NULL,\n"                     \
   "  VALUE         TEXT NOT NULL\n"                         \
   ");\n"                                                    \
   "CREATE UNIQUE INDEX IF NOT EXISTS KEYS_IX ON KEYS (\n"   \
   "  USERID,\n"                                             \
   "  KEYNAME\n"                                             \
   ");"

#define TAGS_SCHEMA                                          \
   "CREATE TABLE IF NOT EXISTS TAGS (\n"                     \
   "  TAGID         INTEGER PRIMARY KEY AUTOINCREMENT,\n"    \
   "  KEYID         INTEGER NOT NULL,\n"                     \
   "  NAME          TEXT NOT NULL,\n"                        \
   "  VALUE         TEXT NOT NULL\n"                         \
   ");\n"                                                    \
   "CREATE UNIQUE INDEX IF NOT EXISTS TAGS_IX ON TAGS (\n"   \
   "  KEYID\n"                                               \
   ");"

typedef struct {
   circus_vault_t fn;
   cad_memory_t memory;
   circus_log_t *log;
   circus_database_t *database;
   cad_hash_t *users;
} vault_impl_t;

typedef struct {
   circus_user_t fn;
   cad_memory_t memory;
   circus_log_t *log;
   int64_t userid;
   uint64_t validity;
   int permissions;
   char *name;
   char *email;
   char *symmkey;
   vault_impl_t *vault;
   cad_hash_t *keys;
   uint64_t stretch;
} user_impl_t;

typedef struct {
   circus_key_t fn;
   cad_memory_t memory;
   circus_log_t *log;
   int64_t keyid;
   user_impl_t *user;
   uint64_t stretch;
} key_impl_t;

user_impl_t *new_vault_user(cad_memory_t memory, circus_log_t *log, int64_t userid, uint64_t validity, int permissions,
                            const char *email, const char *name, vault_impl_t *vault);
key_impl_t *new_vault_key(cad_memory_t memory, circus_log_t *log, int64_t keyid, user_impl_t *user);

user_impl_t *check_user_password(user_impl_t *user, const char *password);
int set_symmetric_key(user_impl_t *user, const char *password);

uint64_t get_stretch_threshold(circus_log_t *log, circus_database_t *database);
int set_stretch_threshold(circus_log_t *log, circus_database_t *database, uint64_t stretch_threshold);

#endif /* __CIRCUS_VAULT_IMPL_H */
