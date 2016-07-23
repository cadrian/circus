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
#include <circus_crypt.h>
#include <circus_log.h>
#include <circus_vault.h>

#include "vault_impl.h"

static char *vault_key_get_password(key_impl_t *this) {
   char *result = NULL;
   char *enckey = this->user->symmkey;
   if (enckey != NULL) {
      static const char *sql = "SELECT SALT, VALUE FROM KEYS WHERE KEYID=?";
      circus_database_query_t *q = this->user->vault->database->query(this->user->vault->database, sql);
      int ok;
      if (q != NULL) {
         ok = q->set_int(q, 0, this->keyid);
         if (ok) {
            circus_database_resultset_t *rs = q->run(q);
            if (rs != NULL) {
               while (rs->has_next(rs)) {
                  rs->next(rs);
                  if (result == NULL) {
                     const char *salt = rs->get_string(rs, 0);
                     const char *value = rs->get_string(rs, 1);
                     char *decvalue = decrypted(this->memory, this->log, value, enckey);
                     if (decvalue != NULL) {
                        result = unsalted(this->memory, this->log, salt, decvalue);
                        this->memory.free(decvalue);
                     }
                  } else {
                     log_error(this->log, "Error: multiple entries for key %"PRId64, this->keyid);
                     this->memory.free(result);
                     result = NULL;
                  }
               }
               rs->free(rs);
            }
         }
         q->free(q);
      }
   }

   return result;
}

static int vault_key_set_password(key_impl_t *this, const char *password) {
   int result = 0;
   char *enckey = this->user->symmkey;
   char *keysalt = NULL;
   char *encpwd = NULL;
   if (enckey == NULL) {
      log_error(this->log, "Could not get symmetric key");
   } else {
      keysalt = salt(this->memory, this->log);
      if (keysalt == NULL) {
         log_error(this->log, "Could not allocate salt");
      } else {
         char *saltedpwd = salted(this->memory, this->log, keysalt, password);
         if (saltedpwd == NULL) {
            log_error(this->log, "Could not allocated salted password");
         }
         encpwd = encrypted(this->memory, this->log, saltedpwd, enckey);
      }
   }
   if (encpwd == NULL) {
      log_error(this->log, "Could not encrypt password");
   } else {
      assert(keysalt != NULL);
      static const char *sql = "UPDATE KEYS SET SALT=?, VALUE=? WHERE KEYID=?";
      circus_database_query_t *q = this->user->vault->database->query(this->user->vault->database, sql);
      if (q != NULL) {
         int ok = 1;
         if (ok) {
            ok = q->set_string(q, 0, keysalt);
         }
         if (ok) {
            ok = q->set_string(q, 1, encpwd);
         }
         if (ok) {
            ok = q->set_int(q, 2, this->keyid);
         }

         if (ok) {
            circus_database_resultset_t *rs = q->run(q);
            if (rs != NULL) {
               result = !rs->has_error(rs);
               rs->free(rs);
            }
         }

         this->memory.free(encpwd);
         this->memory.free(keysalt);

         q->free(q);
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

key_impl_t *new_vault_key(cad_memory_t memory, circus_log_t *log, int64_t keyid, user_impl_t *user) {
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
