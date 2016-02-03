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
#include <sqlite3.h>

/*
 * Because of how the exe resolver works, it is mandatory that the
 * following inclusion is performed before including this file:
 *
 * #include <circus_vault.h>
 */

#define SALT_SIZE 16
#define KEY_SIZE 32 // 256 bits
#define HASH_SIZE 64 // 512 bits

#define USERS_SCHEMA                                            \
   "CREATE TABLE IF NOT EXISTS USERS ("                         \
   "  USERID    INTEGER PRIMARY KEY AUTOINCREMENT,"             \
   "  USERNAME  TEXT NOT NULL,"                                 \
   "  PWDSALT   TEXT NOT NULL,"                                 \
   "  HASHPWD   TEXT NOT NULL,"                                 \
   "  KEYSALT   TEXT NOT NULL,"                                 \
   "  KEY       TEST NOT NULL"                                  \
   ");"                                                         \
   "CREATE UNIQUE INDEX IF NOT EXISTS USERS_IX ON USERS ("      \
   "  USERNAME"                                                 \
   ");"

#define KEYS_SCHEMA                                             \
   "CREATE TABLE IF NOT EXISTS KEYS ("                          \
   "  KEYID    INTEGER PRIMARY KEY AUTOINCREMENT,"              \
   "  USERID   INTEGER NOT NULL, "                              \
   "  KEYNAME  TEXT NOT NULL,"                                  \
   "  SALT     TEXT NOT NULL,"                                  \
   "  VALUE    TEXT NOT NULL"                                   \
   ");"                                                         \
   "CREATE UNIQUE INDEX IF NOT EXISTS KEYS_IX ON KEYS ("        \
   "  USERID,"                                                  \
   "  KEYNAME"                                                  \
   ");"

#define TAGS_SCHEMA                                             \
   "CREATE TABLE IF NOT EXISTS TAGS ("                          \
   "  TAGID    INTEGER PRIMARY KEY AUTOINCREMENT,"              \
   "  KEYID    INTEGER NOT NULL,"                               \
   "  NAME     TEXT NOT NULL,"                                  \
   "  VALUE    TEXT NOT NULL"                                   \
   ");"                                                         \
   "CREATE UNIQUE INDEX IF NOT EXISTS TAGS_IX ON TAGS ("        \
   "  KEYID"                                                    \
   ");"

typedef struct {
   circus_vault_t fn;
   cad_memory_t memory;
   circus_log_t *log;
   sqlite3 *db;
   cad_hash_t *users;
} vault_impl_t;

typedef struct {
   circus_user_t fn;
   cad_memory_t memory;
   circus_log_t *log;
   sqlite3_int64 userid;
   vault_impl_t *vault;
   cad_hash_t *keys;
} user_impl_t;

typedef struct {
   circus_key_t fn;
   cad_memory_t memory;
   circus_log_t *log;
   sqlite3_int64 keyid;
   user_impl_t *user;
} key_impl_t;

user_impl_t *new_vault_user(cad_memory_t memory, circus_log_t *log, sqlite3_int64 userid, vault_impl_t *vault);
key_impl_t *new_vault_key(cad_memory_t memory, circus_log_t *log, sqlite3_int64 keyid, user_impl_t *user);

user_impl_t *check_user_password(user_impl_t *user, const char *password);

/* Encryption routines */

char *salt(cad_memory_t memory, circus_log_t *log);
char *salted(cad_memory_t memory, circus_log_t *log, const char *salt, const char *value);
char *unsalted(cad_memory_t memory, circus_log_t *log, const char *salt, const char *value);
char *hashed(cad_memory_t memory, circus_log_t *log, const char *value);
char *new_symmetric_key(cad_memory_t memory, circus_log_t *log);
char *encrypted(cad_memory_t memory, circus_log_t *log, const char *value, const char *key);
char *decrypted(cad_memory_t memory, circus_log_t *log, const char *b64value, const char *key);

#endif /* __CIRCUS_VAULT_IMPL_H */
