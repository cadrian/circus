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
      log_error(this->log, "User %"PRId64" does not have permission to get keys", this->userid);
      return NULL;
   }

   key_impl_t *result = this->keys->get(this->keys, keyname);
   if (result == NULL) {
      static const char *sql = "SELECT KEYID FROM KEYS WHERE USERID=? AND KEYNAME=?";
      circus_database_query_t *q = this->vault->database->query(this->vault->database, sql);
      int ok;
      if (q != NULL) {
         ok = q->set_int(q, 0, this->userid);
         if (ok) {
            ok = q->set_string(q, 1, keyname);
            if (ok) {
               circus_database_resultset_t *rs = q->run(q);
               if (rs != NULL) {
                  while (rs->has_next(rs)) {
                     rs->next(rs);
                     if (result != NULL) {
                        log_error(this->log, "Error: multiple entries for user %"PRId64" key %s", this->userid, keyname);
                        result->fn.free(&(result->fn));
                        result = NULL;
                     } else {
                        int64_t keyid = rs->get_int(rs, 0);
                        result = new_vault_key(this->memory, this->log, keyid, this);
                     }
                  }
                  rs->free(rs);
               }
            }
         }
         q->free(q);
      }
      if (result != NULL) {
         this->keys->set(this->keys, keyname, result);
      }
   }
   return result;
}

static cad_hash_t *vault_user_get_all(user_impl_t *this) {
   if (((this->permissions) & PERMISSION_USER) == 0) {
      log_error(this->log, "User %"PRId64" does not have permission to get keys", this->userid);
      return NULL;
   }

   return this->keys;
}

static key_impl_t *vault_user_new(user_impl_t *this, const char *keyname) {
   assert(vault_user_get(this, keyname) == NULL);

   if (((this->permissions) & PERMISSION_USER) == 0) {
      log_error(this->log, "User %"PRId64" does not have permission to get keys", this->userid);
      return NULL;
   }

   key_impl_t *result = NULL;
   static const char *sql = "INSERT INTO KEYS (USERID, KEYNAME, SALT, VALUE) VALUES (?, ?, \"\", \"\")";
   circus_database_query_t *q = this->vault->database->query(this->vault->database, sql);
   int ok;
   if (q != NULL) {
      ok = q->set_int(q, 0, this->userid);
      if (ok) {
         ok = q->set_string(q, 1, keyname);
         if (ok) {
            circus_database_resultset_t *rs = q->run(q);
            if (rs != NULL) {
               if (!rs->has_error(rs)) {
                  result = vault_user_get(this, keyname);
               }
               rs->free(rs);
            }
         }
      }
      q->free(q);
   }

   return result;
}

static const char *vault_user_name(user_impl_t *this) {
   return this->name;
}

static int vault_user_set_password(user_impl_t *this, const char *password, uint64_t validity) {
   static const char *sql = "UPDATE USERS SET PWDSALT=?, HASHPWD=?, PWDVALID=? WHERE USERID=?";
   circus_database_query_t *q = this->vault->database->query(this->vault->database, sql);
   int result = 0;
   if (q != NULL) {
      int ok = 1;
      char *pwdsalt=NULL;
      if (ok) {
         pwdsalt = salt(this->memory, this->log);
         if (pwdsalt == NULL) {
            ok = 0;
         } else {
            ok = q->set_string(q, 0, pwdsalt);
         }
      }
      char *pwd=NULL;
      char *hashpwd=NULL;
      if (ok) {
         pwd = salted(this->memory, this->log, pwdsalt, password);
         if (pwd == NULL) {
            ok = 0;
         } else {
            hashpwd = hashed(this->memory, this->log, pwd);
            if (hashpwd == NULL) {
               ok = 0;
            } else {
               ok = q->set_string(q, 1, hashpwd);
            }
         }
      }
      if (ok) {
         ok = q->set_int(q, 2, validity);
      }
      if (ok) {
         ok = q->set_int(q, 3, this->userid);
      }

      if (ok) {
         circus_database_resultset_t *rs = q->run(q);
         if (rs != NULL) {
            if (!rs->has_error(rs)) {
               this->validity = validity;
               result = 1;
            }
            rs->free(rs);
         }
      }

      this->memory.free(hashpwd);
      this->memory.free(pwd);
      this->memory.free(pwdsalt);

      q->free(q);

      if (ok) {
         ok = set_symmetric_key(this, password);
         if (!ok) {
            log_error(this->log, "Symmetric key encryption update failed. The user %"PRId64" may need help.", this->userid);
         }
      }
   }

   return result;
}

static int vault_user_set_email(user_impl_t *this, const char *email) {
   static const char *sql = "UPDATE USERS SET EMAIL=? WHERE USERID=?";
   int result = 0;

   circus_database_query_t *q = this->vault->database->query(this->vault->database, sql);
   int ok;
   if (q != NULL) {
      ok = q->set_string(q, 0, email);
      if (ok) {
         ok = q->set_int(q, 1, this->userid);
         if (ok) {
            circus_database_resultset_t *rs = q->run(q);
            if (rs != NULL) {
               while (rs->has_next(rs)) {
                  rs->next(rs);
               }
               if (!rs->has_error(rs)) {
                  this->memory.free(this->email);
                  this->email = email == NULL ? NULL : szprintf(this->memory, NULL, "%s", email);
                  result = 1;
               }
               rs->free(rs);
            }
         }
      }

      q->free(q);
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

user_impl_t *new_vault_user(cad_memory_t memory, circus_log_t *log, int64_t userid, uint64_t validity, int permissions,
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
      result->symmkey = NULL;
      result->validity = validity;
      strcpy(result->name, name);
   }
   return result;
}

user_impl_t *check_user_password(user_impl_t *user, const char *password) {
   log_debug(user->log, "Checking user %"PRId64" password", user->userid);
   static const char *sql = "SELECT USERNAME, PWDSALT, HASHPWD, PWDVALID FROM USERS WHERE USERID=?";
   user_impl_t *result = NULL;
   circus_database_query_t *q = user->vault->database->query(user->vault->database, sql);
   int ok;
   if (q != NULL) {
      ok = q->set_int(q, 0, user->userid);
      if (ok) {
         circus_database_resultset_t *rs = q->run(q);
         if (rs == NULL) {
            log_error(user->log, "Error user not found: %"PRId64, user->userid);
         } else if (!rs->has_next(rs)) {
            log_error(user->log, "Error user not found: %"PRId64, user->userid);
            rs->free(rs);
         } else {
            rs->next(rs);

            const char *pwdsalt = rs->get_string(rs, 1);
            const char *hashpwd = rs->get_string(rs, 2);
            uint64_t validity = (uint64_t)rs->get_int(rs, 3);
            if ((validity == 0) || ((time_t)validity > now().tv_sec)) {
               char *saltedpwd = salted(user->memory, user->log, pwdsalt, password);
               char *hashedpwd = hashed(user->memory, user->log, saltedpwd);
               if (strcmp(hashedpwd, hashpwd) == 0) {
                  result = user;
               } else {
                  log_warning(user->log, "Invalid password for user %s", rs->get_string(rs, 0));
               }
               user->memory.free(hashedpwd);
               user->memory.free(saltedpwd);
            } else {
               log_warning(user->log, "Stale password for user %s", rs->get_string(rs, 0));
            }
            if (rs->has_next(rs)) {
               log_error(user->log, "Error multiple users found: %"PRId64, user->userid);
               result = NULL;
            }
            rs->free(rs);
         }
      }
      q->free(q);
   }
   return result;
}
