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

#include <string.h>

#include <circus_base64.h>
#include <circus_log.h>
#include <circus_vault.h>

#include "vault_impl.h"

static char *get_symmetric_key(user_impl_t *user) {
   char *result = NULL;
   static const char *sql = "SELECT KEYSALT, KEY, HASHPWD FROM USERS WHERE USERID=?";
   sqlite3_stmt *stmt;
   int n = sqlite3_prepare_v2(user->vault->db, sql, -1, &stmt, NULL);
   if (n != SQLITE_OK) {
      log_error(user->log, "vault_key", "Error preparing statement: %s -- %s", sql, sqlite3_errstr(n));
   } else {
      n = sqlite3_bind_int64(stmt, 1, user->userid);
      if (n != SQLITE_OK) {
         log_error(user->log, "vault_key", "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
      } else {
         n = sqlite3_step(stmt);
         if (n == SQLITE_DONE) {
            log_error(user->log, "vault_key", "Error user not found: %ld -- %s", (long int)user->userid, sqlite3_errstr(n));
         } else if (n != SQLITE_OK) {
            log_error(user->log, "vault_key", "Error user: %ld -- %s", (long int)user->userid, sqlite3_errstr(n));
         } else {
            const char *keysalt = (const char*)sqlite3_column_text(stmt, 1);
            const char *hashkey = (const char*)sqlite3_column_text(stmt, 2);
            const char *hashpwd = (const char*)sqlite3_column_text(stmt, 3);

            char *saltedkey = decrypted(user->memory, user->log, hashkey, hashpwd);
            if (saltedkey != NULL) {
               char *b64key = unsalted(user->memory, user->log, keysalt, saltedkey);
               if (b64key != NULL) {
                  result = unbase64(user->memory, b64key);
                  user->memory.free(b64key);
               }
               user->memory.free(saltedkey);
            }
         }
      }
      n = sqlite3_finalize(stmt);
      if (n != SQLITE_OK) {
         log_warning(user->log, "vault", "Error in finalize: %s", sqlite3_errstr(n));
      }
   }

   if (result == NULL) {
      log_error(user->log, "vault_key", "Could not get encryption key for user %ld", (long int)user->userid);
   }

   return result;
}

static char *vault_key_get_password(key_impl_t *this) {
   char *result = NULL;
   char *enckey = get_symmetric_key(this->user);
   if (enckey != NULL) {
      static const char *sql = "SELECT SALT, VALUE FROM KEYS WHERE KEYID=?";
      sqlite3_stmt *stmt;
      int n = sqlite3_prepare_v2(this->user->vault->db, sql, -1, &stmt, NULL);
      if (n != SQLITE_OK) {
         log_error(this->log, "vault_key", "Error preparing statement: %s -- %s", sql, sqlite3_errstr(n));
      } else {
         n = sqlite3_bind_int64(stmt, 1, this->keyid);
         if (n != SQLITE_OK) {
            log_error(this->log, "vault_key", "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
         } else {
            int done = 0;
            do {
               n = sqlite3_step(stmt);
               switch(n) {
               case SQLITE_OK:
                  if (result != NULL) {
                     log_error(this->log, "vault_key", "Error: multiple entries for key %ld", (long int)this->keyid);
                     this->memory.free(result);
                     result = NULL;
                     done = 1;
                  } else {
                     const char *salt = (const char*)sqlite3_column_text(stmt, 1);
                     const char *value = (const char*)sqlite3_column_text(stmt, 2);

                     char *decvalue = decrypted(this->memory, this->log, value, enckey);
                     if (decvalue != NULL) {
                        result = unsalted(this->memory, this->log, salt, decvalue);
                        this->memory.free(decvalue);
                     }
                  }
                  break;
               case SQLITE_DONE:
                  done = 1;
                  break;
               default:
                  log_error(this->log, "vault_key", "Error stepping statement: %s -- %s", sql, sqlite3_errstr(n));
                  done = 1;
               }
            } while (!done);
         }
      }
   }

   return result;
}

static int vault_key_set_password(key_impl_t *this, const char *password) {
   int result = 0;
   char *enckey = get_symmetric_key(this->user);
   char *keysalt = NULL;
   char *encpwd = NULL;
   if (enckey != NULL) {
      keysalt = salt(this->memory, this->log);
      if (keysalt == NULL) {
         log_error(this->log, "vault_key", "Could not allocate salt");
      } else {
         char *saltedpwd = salted(this->memory, this->log, keysalt, password);
         if (saltedpwd == NULL) {
            log_error(this->log, "vault_key", "Could not allocated salted password");
         }
         encpwd = encrypted(this->memory, this->log, saltedpwd, enckey);
      }
   }
   if (encpwd == NULL) {
      log_error(this->log, "vault_key", "Could not encrypt password");
   } else {
      assert(keysalt != NULL);
      static const char *sql = "UPDATE KEYS SET SALT=?, VALUE=? WHERE KEYID=?";
      sqlite3_stmt *stmt;
      int n = sqlite3_prepare_v2(this->user->vault->db, sql, -1, &stmt, NULL);
      if (n != SQLITE_OK) {
         log_error(this->log, "vault_key", "Error preparing statement: %s -- %s", sql, sqlite3_errstr(n));
      } else {
         int ok=1;
         if (ok) {
            n = sqlite3_bind_text(stmt, 1, keysalt, -1, SQLITE_TRANSIENT);
            if (n != SQLITE_OK) {
               log_error(this->log, "vault_key", "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
               ok=0;
            }
         }
         if (ok) {
            n = sqlite3_bind_text(stmt, 2, encpwd, -1, SQLITE_TRANSIENT);
            if (n != SQLITE_OK) {
               log_error(this->log, "vault_key", "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
               ok=0;
            }
         }
         if (ok) {
            n = sqlite3_bind_int64(stmt, 3, this->keyid);
            if (n != SQLITE_OK) {
               log_error(this->log, "vault_key", "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
               ok=0;
            }
         }

         if (ok) {
            n = sqlite3_step(stmt);
            if (n == SQLITE_OK || n == SQLITE_DONE) {
               result = 1;
            } else {
               log_error(this->log, "vault_key", "Error stepping statement: %s -- %s", sql, sqlite3_errstr(n));
            }
         }

         this->memory.free(encpwd);
         this->memory.free(keysalt);

         n = sqlite3_finalize(stmt);
         if (n != SQLITE_OK) {
            log_warning(this->log, "vault_key", "Error in finalize: %s", sqlite3_errstr(n));
         }
      }
   }
   return result;
}

static void vault_key_free(key_impl_t *this) {
   this->memory.free(this);
}

static circus_key_t vault_key_fn = {
   (circus_key_get_password_fn)vault_key_get_password,
   (circus_key_set_password_fn)vault_key_set_password,
   (circus_key_free_fn)vault_key_free,
};

key_impl_t *new_vault_key(cad_memory_t memory, circus_log_t *log, sqlite3_int64 keyid, user_impl_t *user) {
   key_impl_t *result = memory.malloc(sizeof(key_impl_t));
   if (result != NULL) {
      result->fn = vault_key_fn;
      result->memory = memory;
      result->log = log;
      result->keyid = keyid;
      result->user = user;
   }
   return result;
}
