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
*/

#include <string.h>

#include "circus.h"
#include "log.h"
#include "vault_impl.h"
#include "xdg.h"

extern circus_log_t *LOG;

static user_impl_t *vault_get(vault_impl_t *this, const char *username, const char *password) {
   assert(username != NULL);
   assert(username[0] != 0);
   assert(password != NULL);
   assert(password[0] != 0);

   user_impl_t *result = this->users->get(this->users, username);
   if (result == NULL) {
      const char *sql = "SELECT USERID FROM USERS WHERE USERNAME=?";
      sqlite3_stmt *stmt;
      int n = sqlite3_prepare_v2(this->db, sql, -1, &stmt, NULL);
      if (n != SQLITE_OK) {
         log_error(LOG, "vault", "Error preparing statement: %s -- %s", sql, sqlite3_errstr(n));
      } else {
         n = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
         if (n != SQLITE_OK) {
            log_error(LOG, "vault", "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
         } else {
            int done = 0;
            do {
               n = sqlite3_step(stmt);
               switch(n) {
               case SQLITE_OK:
                  if (result != NULL) {
                     log_error(LOG, "vault", "Error: multiple entries for user %s", username);
                     result->fn.free(&(result->fn));
                     result = NULL;
                     done = 1;
                  } else {
                     sqlite3_int64 userid = sqlite3_column_int64(stmt, 0);
                     result = new_vault_user(this->memory, userid, this);
                  }
                  break;
               case SQLITE_DONE:
                  done = 1;
                  break;
               default:
                  log_error(LOG, "vault", "Error stepping statement: %s -- %s", sql, sqlite3_errstr(n));
                  done = 1;
               }
            } while (!done);
         }
      }
      n = sqlite3_finalize(stmt);
      if (n != SQLITE_OK) {
         log_warning(LOG, "vault", "Error in finalize: %s", sqlite3_errstr(n));
      }
      this->users->set(this->users, username, result);
   }
   return result == NULL ? NULL : check_user(this, result, password);
}

static user_impl_t *vault_new(vault_impl_t *this, const char *username, const char *password) {
   assert(vault_get(this, username, password) == NULL);

   user_impl_t *result = NULL;
   const char *sql = "INSERT INTO USERS (USERNAME, PWDSALT, HASHPWD, KEYSALT, KEY) values (?, ?, ?, ?, ?)";
   sqlite3_stmt *stmt;
   int n = sqlite3_prepare_v2(this->db, sql, -1, &stmt, NULL);
   if (n != SQLITE_OK) {
      log_error(LOG, "vault", "Error preparing statement: %s -- %s", sql, sqlite3_errstr(n));
   } else {
      int ok=1;
      if (ok) {
         n = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
         if (n != SQLITE_OK) {
            log_error(LOG, "vault", "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
            ok=0;
         }
      }
      char *pwdsalt=NULL;
      if (ok) {
         pwdsalt = salt(this->memory);
         if (pwdsalt == NULL) {
            ok=0;
         } else {
            n = sqlite3_bind_text(stmt, 2, pwdsalt, -1, SQLITE_TRANSIENT);
            if (n != SQLITE_OK) {
               log_error(LOG, "vault", "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
               ok=0;
            }
         }
      }
      char *pwd=NULL;
      char *hashpwd=NULL;
      if (ok) {
         pwd = salted(this->memory, pwdsalt, password);
         if (pwd == NULL) {
            ok=0;
         } else {
            hashpwd = hashed(this->memory, pwd);
            if (hashpwd == NULL) {
               ok=0;
            } else {
               n = sqlite3_bind_text(stmt, 3, hashpwd, -1, SQLITE_TRANSIENT);
               if (n != SQLITE_OK) {
                  log_error(LOG, "vault", "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
                  ok=0;
               }
            }
         }
      }
      char *keysalt=NULL;
      if (ok) {
         keysalt = salt(this->memory);
         if (keysalt == NULL) {
            ok=0;
         } else {
            n = sqlite3_bind_text(stmt, 4, keysalt, -1, SQLITE_TRANSIENT);
            if (n != SQLITE_OK) {
               log_error(LOG, "vault", "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
               ok=0;
            }
         }
      }
      char *key=NULL;
      char *enckey=NULL;
      if (ok) {
         enckey = new_symmetric_key(this->memory);
         if (enckey == NULL) {
            ok=0;
         } else {
            key = encrypted(this->memory, salted(this->memory, keysalt, enckey), hashpwd);
            if (key == NULL) {
               ok=0;
            } else {
               n = sqlite3_bind_text(stmt, 5, key, -1, SQLITE_TRANSIENT);
               if (n != SQLITE_OK) {
                  log_error(LOG, "vault", "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
                  ok=0;
               }
            }
         }
      }

      if (ok) {
         n = sqlite3_step(stmt);
         if (n == SQLITE_OK || n == SQLITE_DONE) {
            result = vault_get(this, username, password);
         } else {
            log_error(LOG, "vault", "Error stepping statement: %s -- %s", sql, sqlite3_errstr(n));
         }
      }

      n = sqlite3_finalize(stmt);
      if (n != SQLITE_OK) {
         log_warning(LOG, "vault", "Error in finalize: %s", sqlite3_errstr(n));
      }
      this->users->set(this->users, username, result);
   }

   return result;
}

static void vault_clean(cad_hash_t *UNUSED(hash), int UNUSED(index), const char *UNUSED(name), user_impl_t *user, vault_impl_t *UNUSED(vault)) {
   user->fn.free(&(user->fn));
}

static void vault_free(vault_impl_t *this) {
   this->users->clean(this->users, (cad_hash_iterator_fn)vault_clean, this);
   this->users->free(this->users);
   sqlite3_close(this->db);
   this->memory.free(this);
}

static circus_vault_t vault_fn = {
   (circus_vault_get_fn)vault_get,
   (circus_vault_new_fn)vault_new,
   (circus_vault_free_fn)vault_free,
};

circus_vault_t *circus_vault(cad_memory_t memory, circus_config_t *config) {
   vault_impl_t *result = NULL;
   char *path;
   int n;
   char *err;
   const char *filename = config->get(config, "vault", "filename");
   if (filename == NULL || filename[0] == 0) {
      filename = "vault";
   }

   result = memory.malloc(sizeof(vault_impl_t));
   assert(result != NULL);
   result->fn = vault_fn;
   result->memory = memory;

   path = szprintf(memory, NULL, "%s/%s", xdg_data_home(), filename);
   n = sqlite3_open_v2(path, &(result->db),
                       SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_PRIVATECACHE,
                       "unix-excl");
   if (n != SQLITE_OK) {
      log_error(LOG, "vault", "Cannot open database: %s", sqlite3_errmsg(result->db));
      sqlite3_close(result->db);
      memory.free(result);
      return NULL;
   }
   memory.free(path);

   n = sqlite3_exec(result->db, USERS_SCHEMA, NULL, NULL, &err);
   if (n != SQLITE_OK) {
      log_error(LOG, "vault", "Error creating USERS table: %s", err);
      sqlite3_free(err);
      sqlite3_close(result->db);
      memory.free(result);
      return NULL;
   }

   n = sqlite3_exec(result->db, KEYS_SCHEMA, NULL, NULL, &err);
   if (n != SQLITE_OK) {
      log_error(LOG, "vault", "Error creating KEYS table: %s", err);
      sqlite3_free(err);
      sqlite3_close(result->db);
      memory.free(result);
      return NULL;
   }

   n = sqlite3_exec(result->db, TAGS_SCHEMA, NULL, NULL, &err);
   if (n != SQLITE_OK) {
      log_error(LOG, "vault", "Error creating TAGS table: %s", err);
      sqlite3_free(err);
      sqlite3_close(result->db);
      memory.free(result);
      return NULL;
   }

   return (circus_vault_t*)result;
}
