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

#include <circus_log.h>
#include <circus_vault.h>

#include "vault_impl.h"

extern circus_log_t *LOG;

static key_impl_t *vault_user_get(user_impl_t *this, const char *keyname) {
   assert(keyname != NULL);
   assert(keyname[0] != 0);

   key_impl_t *result = this->keys->get(this->keys, keyname);
   if (result == NULL) {
      static const char *sql = "SELECT KEYID FROM KEYS WHERE USERID=? AND KEYNAME=?";
      sqlite3_stmt *stmt;
      int n = sqlite3_prepare_v2(this->vault->db, sql, -1, &stmt, NULL);
      if (n != SQLITE_OK) {
         log_error(LOG, "vault_user", "Error preparing statement: %s -- %s", sql, sqlite3_errstr(n));
      } else {
         n = sqlite3_bind_int64(stmt, 1, this->userid);
         if (n != SQLITE_OK) {
            log_error(LOG, "vault", "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
         } else {
            n = sqlite3_bind_text(stmt, 2, keyname, -1, SQLITE_TRANSIENT);
            if (n != SQLITE_OK) {
               log_error(LOG, "vault", "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
            } else {
               int done = 0;
               do {
                  n = sqlite3_step(stmt);
                  switch(n) {
                  case SQLITE_OK:
                     if (result != NULL) {
                        log_error(LOG, "vault_user", "Error: multiple entries for user %ld key %s", (long int)this->userid, keyname);
                        result->fn.free(&(result->fn));
                        result = NULL;
                        done = 1;
                     } else {
                        sqlite3_int64 keyid = sqlite3_column_int64(stmt, 0);
                        result = new_vault_key(this->memory, keyid, this);
                     }
                     break;
                  case SQLITE_DONE:
                     done = 1;
                     break;
                  default:
                     log_error(LOG, "vault_user", "Error stepping statement: %s -- %s", sql, sqlite3_errstr(n));
                     done = 1;
                  }
               } while (!done);
            }
         }
      }
      n = sqlite3_finalize(stmt);
      if (n != SQLITE_OK) {
         log_warning(LOG, "vault_user", "Error in finalize: %s", sqlite3_errstr(n));
      }
      if (result != NULL) {
         this->keys->set(this->keys, keyname, result);
      }
   }
   return result;
}

static key_impl_t *vault_user_new(user_impl_t *this, const char *keyname) {
   assert(vault_user_get(this, keyname) == NULL);

   key_impl_t *result = NULL;
   static const char *sql = "INSERT INTO KEYS (USERID, KEYNAME, SALT, VALUE) VALUES (?, ?, \"\", \"\")";
   sqlite3_stmt *stmt;
   int n = sqlite3_prepare_v2(this->vault->db, sql, -1, &stmt, NULL);
   if (n != SQLITE_OK) {
      log_error(LOG, "vault_user", "Error preparing statement: %s -- %s", sql, sqlite3_errstr(n));
   } else {
      n = sqlite3_bind_int64(stmt, 1, this->userid);
      if (n != SQLITE_OK) {
         log_error(LOG, "vault_user", "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
      } else {
         n = sqlite3_bind_text(stmt, 2, keyname, -1, SQLITE_TRANSIENT);
         if (n != SQLITE_OK) {
            log_error(LOG, "vault_user", "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
         } else {
            if (n == SQLITE_OK || n == SQLITE_DONE) {
               result = vault_user_get(this, keyname);
            } else {
               log_error(LOG, "vault_user", "Error stepping statement: %s -- %s", sql, sqlite3_errstr(n));
            }
         }
      }
      n = sqlite3_finalize(stmt);
      if (n != SQLITE_OK) {
         log_warning(LOG, "vault", "Error in finalize: %s", sqlite3_errstr(n));
      }
   }

   return result;
}

static void user_clean(cad_hash_t *UNUSED(hash), int UNUSED(index), const char *UNUSED(name), key_impl_t *key, user_impl_t *UNUSED(user)) {
   key->fn.free(&(key->fn));
}

static void vault_user_free(user_impl_t *this) {
   this->keys->clean(this->keys, (cad_hash_iterator_fn)user_clean, this);
   this->keys->free(this->keys);
   this->memory.free(this);
}

static circus_user_t vault_user_fn = {
   (circus_user_get_fn)vault_user_get,
   (circus_user_new_fn)vault_user_new,
   (circus_user_free_fn)vault_user_free,
};

user_impl_t *new_vault_user(cad_memory_t memory, sqlite3_int64 userid, vault_impl_t *vault) {
   user_impl_t *result = memory.malloc(sizeof(user_impl_t));
   if (result != NULL) {
      result->fn = vault_user_fn;
      result->memory = memory;
      result->userid = userid;
      result->vault = vault;
      result->keys = cad_new_hash(memory, cad_hash_strings);
   }
   return result;
}

user_impl_t *check_user_password(user_impl_t *user, const char *password) {
   static const char *sql = "SELECT USERNAME, PWDSALT, HASHPWD FROM USERS WHERE USERID=?";
   sqlite3_stmt *stmt;
   user_impl_t *result = NULL;
   int n = sqlite3_prepare_v2(user->vault->db, sql, -1, &stmt, NULL);
   if (n != SQLITE_OK) {
      log_error(LOG, "vault_user", "Error preparing statement: %s -- %s", sql, sqlite3_errstr(n));
   } else {
      n = sqlite3_bind_int64(stmt, 1, user->userid);
      if (n != SQLITE_OK) {
         log_error(LOG, "vault_user", "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
      } else {
         n = sqlite3_step(stmt);
         if (n == SQLITE_DONE) {
            log_error(LOG, "vault_user", "Error user not found: %ld -- %s", sql, (long int)user->userid, sqlite3_errstr(n));
         } else if (n != SQLITE_OK) {
            log_error(LOG, "vault_user", "Error user: %ld -- %s", sql, (long int)user->userid, sqlite3_errstr(n));
         } else {
            const char *pwdsalt = (const char*)sqlite3_column_text(stmt, 1);
            const char *hashpwd = (const char*)sqlite3_column_text(stmt, 2);

            char *saltedpwd = salted(user->memory, pwdsalt, password);
            char *hashedpwd = hashed(user->memory, saltedpwd);
            if (strcmp(hashedpwd, hashpwd) == 0) {
               result = user;
            } else {
               log_warning(LOG, "vault_user", "Invalid password for user %s", sqlite3_column_text(stmt, 0));
            }
            user->memory.free(hashedpwd);
            user->memory.free(saltedpwd);

            n = sqlite3_step(stmt);
            if (n != SQLITE_DONE) {
               log_error(LOG, "vault_user", "Error multiple users not found: %ld -- %s", sql, (long int)user->userid, sqlite3_errstr(n));
               result = NULL;
            }
         }
      }
      n = sqlite3_finalize(stmt);
      if (n != SQLITE_OK) {
         log_warning(LOG, "vault_user", "Error in finalize: %s", sqlite3_errstr(n));
      }
   }
   return result;
}
