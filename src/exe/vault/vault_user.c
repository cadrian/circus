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

#include <circus_crypt.h>
#include <circus_log.h>
#include <circus_time.h>
#include <circus_vault.h>

#include "vault_impl.h"

static key_impl_t *vault_user_get(user_impl_t *this, const char *keyname) {
   assert(keyname != NULL);
   assert(keyname[0] != 0);

   if (((this->permissions) & PERMISSION_USER) == 0) {
      log_error(this->log, "User %ld does not have permission to get keys", (long int)this->userid);
      return NULL;
   }

   key_impl_t *result = this->keys->get(this->keys, keyname);
   if (result == NULL) {
      static const char *sql = "SELECT KEYID FROM KEYS WHERE USERID=? AND KEYNAME=?";
      sqlite3_stmt *stmt;
      int n = sqlite3_prepare_v2(this->vault->db, sql, -1, &stmt, NULL);
      if (n != SQLITE_OK) {
         log_error(this->log, "Error preparing statement: %s -- %s", sql, sqlite3_errstr(n));
      } else {
         n = sqlite3_bind_int64(stmt, 1, this->userid);
         if (n != SQLITE_OK) {
            log_error(this->log, "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
         } else {
            n = sqlite3_bind_text(stmt, 2, keyname, -1, SQLITE_TRANSIENT);
            if (n != SQLITE_OK) {
               log_error(this->log, "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
            } else {
               int done = 0;
               do {
                  n = sqlite3_step(stmt);
                  switch(n) {
                  case SQLITE_OK:
                  case SQLITE_ROW:
                     if (result != NULL) {
                        log_error(this->log, "Error: multiple entries for user %ld key %s", (long int)this->userid, keyname);
                        result->fn.free(&(result->fn));
                        result = NULL;
                        done = 1;
                     } else {
                        sqlite3_int64 keyid = sqlite3_column_int64(stmt, 0);
                        result = new_vault_key(this->memory, this->log, keyid, this);
                     }
                     break;
                  case SQLITE_DONE:
                     done = 1;
                     break;
                  default:
                     log_error(this->log, "Error stepping statement: %s -- %s", sql, sqlite3_errstr(n));
                     done = 1;
                  }
               } while (!done);
            }
         }
      }
      n = sqlite3_finalize(stmt);
      if (n != SQLITE_OK) {
         log_warning(this->log, "Error in finalize: %s", sqlite3_errstr(n));
      }
      if (result != NULL) {
         this->keys->set(this->keys, keyname, result);
      }
   }
   return result;
}

static cad_hash_t *vault_user_get_all(user_impl_t *this) {
   if (((this->permissions) & PERMISSION_USER) == 0) {
      log_error(this->log, "User %ld does not have permission to get keys", (long int)this->userid);
      return NULL;
   }

   return this->keys;
}

static key_impl_t *vault_user_new(user_impl_t *this, const char *keyname) {
   assert(vault_user_get(this, keyname) == NULL);

   if (((this->permissions) & PERMISSION_USER) == 0) {
      log_error(this->log, "User %ld does not have permission to get keys", (long int)this->userid);
      return NULL;
   }

   key_impl_t *result = NULL;
   static const char *sql = "INSERT INTO KEYS (USERID, KEYNAME, SALT, VALUE) VALUES (?, ?, \"\", \"\")";
   sqlite3_stmt *stmt;
   int n = sqlite3_prepare_v2(this->vault->db, sql, -1, &stmt, NULL);
   if (n != SQLITE_OK) {
      log_error(this->log, "Error preparing statement: %s -- %s", sql, sqlite3_errstr(n));
   } else {
      n = sqlite3_bind_int64(stmt, 1, this->userid);
      if (n != SQLITE_OK) {
         log_error(this->log, "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
      } else {
         n = sqlite3_bind_text(stmt, 2, keyname, -1, SQLITE_TRANSIENT);
         if (n != SQLITE_OK) {
            log_error(this->log, "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
         } else {
            n = sqlite3_step(stmt);
            if (n == SQLITE_OK || n == SQLITE_DONE) {
               result = vault_user_get(this, keyname);
            } else {
               log_error(this->log, "Error stepping statement: %s -- %s", sql, sqlite3_errstr(n));
            }
         }
      }
      n = sqlite3_finalize(stmt);
      if (n != SQLITE_OK) {
         log_warning(this->log, "Error in finalize: %s", sqlite3_errstr(n));
      }
   }

   return result;
}

static const char *vault_user_name(user_impl_t *this) {
   return this->name;
}

static int vault_user_set_password(user_impl_t *this, const char *password, uint64_t validity) {
   static const char *sql = "UPDATE USERS SET PWDSALT=?, HASHPWD=?, PWDVALID=? WHERE USERID=?";
   sqlite3_stmt *stmt;
   int result = 0;
   int n = sqlite3_prepare_v2(this->vault->db, sql, -1, &stmt, NULL);
   if (n != SQLITE_OK) {
      log_error(this->log, "Error preparing statement: %s -- %s", sql, sqlite3_errstr(n));
   } else {
      int ok=1;
      char *pwdsalt=NULL;
      if (ok) {
         pwdsalt = salt(this->memory, this->log);
         if (pwdsalt == NULL) {
            ok=0;
         } else {
            n = sqlite3_bind_text(stmt, 1, pwdsalt, -1, SQLITE_TRANSIENT);
            if (n != SQLITE_OK) {
               log_error(this->log, "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
               ok=0;
            }
         }
      }
      char *pwd=NULL;
      char *hashpwd=NULL;
      if (ok) {
         pwd = salted(this->memory, this->log, pwdsalt, password);
         if (pwd == NULL) {
            ok=0;
         } else {
            hashpwd = hashed(this->memory, this->log, pwd);
            if (hashpwd == NULL) {
               ok=0;
            } else {
               n = sqlite3_bind_text(stmt, 2, hashpwd, -1, SQLITE_TRANSIENT);
               if (n != SQLITE_OK) {
                  log_error(this->log, "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
                  ok=0;
               }
            }
         }
      }
      if (ok) {
         n = sqlite3_bind_int64(stmt, 3, validity);
         if (n != SQLITE_OK) {
            log_error(this->log, "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
            ok=0;
         }
      }
      if (ok) {
         n = sqlite3_bind_int64(stmt, 4, this->userid);
         if (n != SQLITE_OK) {
            log_error(this->log, "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
            ok=0;
         }
      }

      if (ok) {
         n = sqlite3_step(stmt);
         if (n == SQLITE_OK || n == SQLITE_DONE) {
            this->validity = (sqlite3_int64)validity;
            result = 1;
         } else {
            log_error(this->log, "Error stepping statement: %s -- %s", sql, sqlite3_errstr(n));
         }
      }

      this->memory.free(hashpwd);
      this->memory.free(pwd);
      this->memory.free(pwdsalt);

      n = sqlite3_finalize(stmt);
      if (n != SQLITE_OK) {
         log_warning(this->log, "Error in finalize: %s", sqlite3_errstr(n));
      }
   }

   return result;
}

static int vault_user_set_email(user_impl_t *this, const char *email) {
   static const char *sql = "UPDATE USERS SET EMAIL=? WHERE USERID=?";
   sqlite3_stmt *stmt;
   int result = 0;

   int n = sqlite3_prepare_v2(this->vault->db, sql, -1, &stmt, NULL);
   if (n != SQLITE_OK) {
      log_error(this->log, "Error preparing statement: %s -- %s", sql, sqlite3_errstr(n));
   } else {
      n = sqlite3_bind_text(stmt, 1, email, -1, SQLITE_TRANSIENT);
      if (n != SQLITE_OK) {
         log_error(this->log, "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
      } else {
         n = sqlite3_bind_int64(stmt, 2, this->userid);
         if (n != SQLITE_OK) {
            log_error(this->log, "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
         } else {
            n = sqlite3_step(stmt);
            if (n == SQLITE_OK || n == SQLITE_DONE) {
               this->memory.free(this->email);
               this->email = email == NULL ? NULL : szprintf(this->memory, NULL, "%s", email);
               result = 1;
            } else {
               log_error(this->log, "Error stepping statement: %s -- %s", sql, sqlite3_errstr(n));
            }
         }
      }

      n = sqlite3_finalize(stmt);
      if (n != SQLITE_OK) {
         log_warning(this->log, "Error in finalize: %s", sqlite3_errstr(n));
      }
   }

   return result;
}

static int vault_user_is_admin(user_impl_t *this) {
   return ((this->permissions) & PERMISSION_ADMIN) != 0;
}

static time_t vault_user_validity(user_impl_t *this) {
   return (time_t)this->validity;
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
   (circus_user_get_all_fn)vault_user_get_all,
   (circus_user_new_fn)vault_user_new,
   (circus_user_name_fn)vault_user_name,
   (circus_user_set_password_fn)vault_user_set_password,
   (circus_user_set_email_fn)vault_user_set_email,
   (circus_user_is_admin_fn)vault_user_is_admin,
   (circus_user_validity_fn)vault_user_validity,
   (circus_user_free_fn)vault_user_free,
};

user_impl_t *new_vault_user(cad_memory_t memory, circus_log_t *log, sqlite3_int64 userid, sqlite3_int64 validity, int permissions,
                            const char *email, const char *name, vault_impl_t *vault) {
   user_impl_t *result = memory.malloc(sizeof(user_impl_t) + strlen(name) + 1);
   if (result != NULL) {
      result->fn = vault_user_fn;
      result->memory = memory;
      result->log = log;
      result->userid = userid;
      result->permissions = permissions;
      result->name = (char*)(result + 1);
      result->vault = vault;
      result->keys = cad_new_hash(memory, cad_hash_strings);
      result->email = email == NULL ? NULL : szprintf(memory, NULL, "%s", email);
      result->key = NULL;
      result->validity = validity;
      strcpy(result->name, name);
   }
   return result;
}

user_impl_t *check_user_password(user_impl_t *user, const char *password) {
   static const char *sql = "SELECT USERNAME, PWDSALT, HASHPWD, PWDVALID FROM USERS WHERE USERID=?";
   sqlite3_stmt *stmt;
   user_impl_t *result = NULL;
   int n = sqlite3_prepare_v2(user->vault->db, sql, -1, &stmt, NULL);
   if (n != SQLITE_OK) {
      log_error(user->log, "Error preparing statement: %s -- %s", sql, sqlite3_errstr(n));
   } else {
      n = sqlite3_bind_int64(stmt, 1, user->userid);
      if (n != SQLITE_OK) {
         log_error(user->log, "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
      } else {
         n = sqlite3_step(stmt);
         if (n == SQLITE_DONE) {
            log_error(user->log, "Error user not found: %ld -- %s", (long int)user->userid, sqlite3_errstr(n));
         } else if (n != SQLITE_OK && n != SQLITE_ROW) {
            log_error(user->log, "Error user: %ld -- %s", (long int)user->userid, sqlite3_errstr(n));
         } else {
            const char *pwdsalt = (const char*)sqlite3_column_text(stmt, 1);
            const char *hashpwd = (const char*)sqlite3_column_text(stmt, 2);
            sqlite3_int64 validity = sqlite3_column_int64(stmt, 3);
            if ((validity == 0) || ((time_t)validity > now().tv_sec)) {
               char *saltedpwd = salted(user->memory, user->log, pwdsalt, password);
               char *hashedpwd = hashed(user->memory, user->log, saltedpwd);
               if (strcmp(hashedpwd, hashpwd) == 0) {
                  result = user;
               } else {
                  log_warning(user->log, "Invalid password for user %s", sqlite3_column_text(stmt, 0));
               }
               user->memory.free(hashedpwd);
               user->memory.free(saltedpwd);
            } else {
               log_warning(user->log, "Stale password for user %s", sqlite3_column_text(stmt, 0));
            }
            n = sqlite3_step(stmt);
            if (n != SQLITE_DONE) {
               log_error(user->log, "Error multiple users not found: %ld -- %s", (long int)user->userid, sqlite3_errstr(n));
               result = NULL;
            }
         }
      }
      n = sqlite3_finalize(stmt);
      if (n != SQLITE_OK) {
         log_warning(user->log, "Error in finalize: %s", sqlite3_errstr(n));
      }
   }
   return result;
}
