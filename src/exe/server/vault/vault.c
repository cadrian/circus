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

#include <circus.h>
#include <circus_crypt.h>
#include <circus_log.h>
#include <circus_vault.h>
#include <circus_xdg.h>

#include "vault_impl.h"

/*
 * TODO - WIP password stretching:
 * - stretch user password, but no need for symmkey (to be thought of... hard)
 * - stretch user keys
 */

static void get_symmetric_key(user_impl_t *user, const char *password) {
   assert(password != NULL && password[0] != 0);

   static const char *sql = "SELECT KEYSALT, HASHKEY FROM USERS WHERE USERID=?";
   circus_database_query_t *q = user->vault->database->query(user->vault->database, sql);
   int ok;

   if (q != NULL) {
      ok = q->set_int(q, 0, user->userid);
      if (ok) {
         circus_database_resultset_t *rs = q->run(q);
         if (rs != NULL) {
            if (!rs->has_next(rs)) {
               log_error(user->log, "Error user not found: %"PRId64, user->userid);
               ok = 0;
            } else {
               rs->next(rs);

               const char *keysalt = rs->get_string(rs, 0);
               const char *hashkey = rs->get_string(rs, 1);

               if (hashkey == NULL || strlen(hashkey) == 0) {
                  log_error(user->log, "Error user: %"PRId64" -- HASHKEY is NULL", user->userid);
               } else if (keysalt == NULL || strlen(keysalt) == 0) {
                  log_error(user->log, "Error user: %"PRId64" -- KEYSALT is NULL", user->userid);
               } else {
                  char *passslt = NULL;
                  char *passkey = NULL;
                  passslt = salted(user->memory, user->log, keysalt, password);
                  if (passslt == NULL) {
                     log_error(user->log, "Error user: %"PRId64" -- could not salt password", user->userid);
                  } else {
                     passkey = hashed(user->memory, user->log, passslt);
                     if (passkey == NULL) {
                        log_error(user->log, "Error user: %"PRId64" -- could not hash password", user->userid);
                     } else {
                        char *saltedkey = decrypted(user->memory, user->log, hashkey, passkey);
                        if (saltedkey == NULL) {
                           log_error(user->log, "Error user: %"PRId64" -- could not decrypt", user->userid);
                        } else {
                           user->symmkey = unsalted(user->memory, user->log, keysalt, saltedkey);
                           if (user->symmkey == NULL) {
                              log_error(user->log, "Error user: %"PRId64" -- could not unsalt", user->userid);
                           }
                           user->memory.free(saltedkey);
                        }
                        user->memory.free(passkey);
                     }
                     user->memory.free(passslt);
                  }
               }
            }
            rs->free(rs);
         }
      }
      q->free(q);
   }
}

int set_symmetric_key(user_impl_t *user, const char *password) {
   assert(password != NULL && password[0] != 0);

   // TODO stretch

   int result = 0;
   static const char *sql = "UPDATE USERS SET KEYSALT=?, HASHKEY=? WHERE USERID=?";
   circus_database_query_t *q = user->vault->database->query(user->vault->database, sql);
   if (q != NULL) {
      int ok = 1;
      char *keysalt = NULL;
      if (ok) {
         keysalt = salt(user->memory, user->log);
         if (keysalt == NULL) {
            ok = 0;
         } else {
            ok = q->set_string(q, 0, keysalt);
         }
      }

      char *hashkey = NULL;
      char *enckey = NULL;
      char *sltkey = NULL;
      char *passslt = NULL;
      char *passkey = NULL;
      if (ok) {
         if (user->symmkey == NULL) {
            user->symmkey = new_symmetric_key(user->memory, user->log);
         }
         enckey = szprintf(user->memory, NULL, "%s", user->symmkey);
         if (enckey == NULL) {
            ok = 0;
         } else {
            sltkey = salted(user->memory, user->log, keysalt, enckey);
            if (sltkey == NULL) {
               ok = 0;
            } else {
               passslt = salted(user->memory, user->log, keysalt, password);
               if (passslt == NULL) {
                  ok = 0;
               } else {
                  passkey = hashed(user->memory, user->log, passslt);
                  if (passkey == NULL) {
                     ok = 0;
                  } else {
                     // HERE is the reason why KEY_SIZE == HASH_SIZE in crypt.c
                     hashkey = encrypted(user->memory, user->log, sltkey, passkey);
                     if (hashkey == NULL) {
                        ok = 0;
                     } else {
                        ok = q->set_string(q, 1, hashkey);
                     }
                  }
               }
            }
         }
      }

      if (ok) {
         ok = q->set_int(q, 2, user->userid);
      }

      if (ok) {
         circus_database_resultset_t *rs = q->run(q);
         if (rs != NULL) {
            result = !rs->has_error(rs);
            rs->free(rs);
         }
      }

      user->memory.free(sltkey);
      user->memory.free(enckey);
      user->memory.free(hashkey);
      user->memory.free(keysalt);
      user->memory.free(passslt);
      user->memory.free(passkey);

      q->free(q);
   }

   return result;
}

static user_impl_t *vault_get_(vault_impl_t *this, const char *username, const char *password, int with_symmkey) {
   log_info(this->log, "Getting user %s%s%s", username,
            with_symmkey ? " with their symmetric key" : "",
            password == NULL ? (with_symmkey ? " BUT MISSING PASSWORD" : "") : ", and checking password");

   user_impl_t *result = this->users->get(this->users, username);
   if (result == NULL) {
      log_debug(this->log, "User %s not found in dict, loading from database", username);
      static const char *sql = "SELECT USERID, PERMISSIONS, EMAIL, PWDVALID FROM USERS WHERE USERNAME=?";
      circus_database_query_t *q = this->database->query(this->database, sql);
      int ok;
      if (q != NULL) {
         ok = q->set_string(q, 0, username);
         if (ok) {
            circus_database_resultset_t *rs = q->run(q);
            if (rs != NULL) {
               while (rs->has_next(rs)) {
                  rs->next(rs);
                  if (result != NULL) {
                     log_error(this->log, "Error: multiple entries for user %s", username);
                     result->fn.free(&(result->fn));
                     result = NULL;
                  } else {
                     int64_t userid = rs->get_int(rs, 0);
                     int permissions = (int)rs->get_int(rs, 1);
                     const char *email = rs->get_string(rs, 2);
                     uint64_t validity = (uint64_t)rs->get_int(rs, 3);
                     result = new_vault_user(this->memory, this->log, userid, validity, permissions, email, username, this);
                  }
               }
               rs->free(rs);
            }
         }
         q->free(q);
      }
      if (result != NULL) {
         log_debug(this->log, "Updating users cache");
         this->users->set(this->users, username, result);
         if (with_symmkey && password != NULL && password[0] != 0) {
            log_debug(this->log, "Getting user symmetric key");
            get_symmetric_key(result, password);
         }
      }
   } else if (with_symmkey && result->symmkey == NULL && password != NULL && password[0] != 0) {
      log_debug(this->log, "Getting user symmetric key");
      get_symmetric_key(result, password);
   }

   if (result == NULL) {
      log_error(this->log, "Could not find user %s", username);
   } else {
      log_pii(this->log, "User %s has userid %"PRId64, username, result->userid);
      if (password != NULL && password[0] != 0) {
         log_debug(this->log, "Checking user password");
         result = check_user_password(result, password);
      }
   }
   return result;
}

static user_impl_t *vault_get(vault_impl_t *this, const char *username, const char *password) {
   assert(username != NULL);
   assert(username[0] != 0);
   user_impl_t *result = vault_get_(this, username, password, 1);
   if (result == NULL) {
      log_warning(this->log, "User not found: %s", username);
   }
   return result;
}

static user_impl_t *vault_new_(vault_impl_t *this, const char *username, const char *password, uint64_t validity, int permissions) {
   assert(password != NULL && password[0] != 0);
   log_info(this->log, "Creating new user %s", username);

   user_impl_t *result = NULL;
   static const char *sql = "INSERT INTO USERS (USERNAME, PERMISSIONS, STRETCH, PWDSALT, HASHPWD, PWDVALID, KEYSALT, HASHKEY) "
      "values (?, ?, 0, ?, ?, ?, 'invalid', 'invalid')";
   circus_database_query_t *q = this->database->query(this->database, sql);

   if (q != NULL) {
      int ok = 1;
      if (ok) {
         ok = q->set_string(q, 0, username);
      }
      if (ok) {
         ok = q->set_int(q, 1, permissions);
      }
      char *pwdsalt=NULL;
      if (ok) {
         pwdsalt = salt(this->memory, this->log);
         if (pwdsalt == NULL) {
            log_error(this->log, "Error creating salt");
            ok = 0;
         } else {
            ok = q->set_string(q, 2, pwdsalt);
         }
      }
      char *pwd=NULL;
      char *hashpwd=NULL;
      if (ok) {
         pwd = salted(this->memory, this->log, pwdsalt, password);
         if (pwd == NULL) {
            log_error(this->log, "Error salting password");
            ok = 0;
         } else {
            hashpwd = hashed(this->memory, this->log, pwd);
            if (hashpwd == NULL) {
               log_error(this->log, "Error hashing password");
               ok = 0;
            } else {
               ok = q->set_string(q, 3, hashpwd);
            }
         }
      }
      if (ok) {
         ok = q->set_int(q, 4, validity);
      }

      if (ok) {
         circus_database_resultset_t *rs = q->run(q);
         if (rs != NULL) {
            rs->free(rs);
         }
      }

      this->memory.free(hashpwd);
      this->memory.free(pwd);
      this->memory.free(pwdsalt);

      q->free(q);

      if (ok) {
         result = vault_get_(this, username, password, 0);
         if (result == NULL) {
            log_error(this->log, "Error getting the just-created user from the database");
            ok = 0;
         } else {
            log_debug(this->log, "Creating user symmetric key");
            ok = set_symmetric_key(result, password);
            if (!ok) {
               log_error(this->log, "Symmetric key encryption creation failed. The user %"PRId64" may need help.", result->userid);
            }
         }
      }
   }

   return result;
}

static user_impl_t *vault_new(vault_impl_t *this, const char *username, const char *password, uint64_t validity) {
   assert(password != NULL && password[0] != 0);
   assert(vault_get_(this, username, password, 0) == NULL);
   return vault_new_(this, username, password, validity, PERMISSION_USER);
}

static int vault_install(vault_impl_t *this, const char *admin_username, const char *admin_password) {
   assert(admin_password != NULL && admin_password[0] != 0);

   int status = 0;
   int ok;

   ok = database_exec(this->log, this->database, META_SCHEMA);
   if (!ok) {
      log_error(this->log, "Error creating META table");
      status = 1;
   }

   ok = database_exec(this->log, this->database, USERS_SCHEMA);
   if (!ok) {
      log_error(this->log, "Error creating USERS table");
      status = 1;
   }

   ok = database_exec(this->log, this->database, KEYS_SCHEMA);
   if (!ok) {
      log_error(this->log, "Error creating KEYS table");
      status = 1;
   }

   ok = database_exec(this->log, this->database, TAGS_SCHEMA);
   if (!ok) {
      log_error(this->log, "Error creating TAGS table");
      status = 1;
   }

   static const char *sql = "INSERT INTO META (KEY, VALUE) VALUES ('VERSION', ?)";
   circus_database_query_t *q = this->database->query(this->database, sql);
   if (q == NULL) {
      status = 1;
   } else {
      ok = q->set_string(q, 0, DB_VERSION);
      if (!ok) {
         status = 1;
      } else {
         circus_database_resultset_t *rs = q->run(q);
         if (rs == NULL) {
            status = 1;
         } else {
            status = !!rs->has_error(rs);
            rs->free(rs);
         }
      }
      q->free(q);
   }

   user_impl_t *admin = vault_get_(this, admin_username, NULL, 1);
   if (admin == NULL) {
      admin = vault_new_(this, admin_username, admin_password, 0, PERMISSION_ADMIN);
   } else {
      log_warning(this->log, "User %s already exists, ignoring password change", admin_username);
   }

   return status;
}

static void vault_clean(cad_hash_t *UNUSED(hash), int UNUSED(index), const char *UNUSED(name), user_impl_t *user, vault_impl_t *UNUSED(vault)) {
   user->fn.free(&(user->fn));
}

static void vault_free(vault_impl_t *this) {
   this->users->clean(this->users, (cad_hash_iterator_fn)vault_clean, this);
   this->users->free(this->users);
   this->database->free(this->database);
   this->memory.free(this);
}

static circus_vault_t vault_fn = {
   (circus_vault_get_fn)vault_get,
   (circus_vault_new_fn)vault_new,
   (circus_vault_install_fn)vault_install,
   (circus_vault_free_fn)vault_free,
};

circus_vault_t *circus_vault(cad_memory_t memory, circus_log_t *log, circus_config_t *config, database_factory_fn db_factory) {
   vault_impl_t *result = NULL;
   char *path;
   const char *filename = config->get(config, "vault", "filename");

   result = memory.malloc(sizeof(vault_impl_t));
   assert(result != NULL);
   result->fn = vault_fn;
   result->memory = memory;
   result->log = log;
   result->users = cad_new_hash(memory, cad_hash_strings);

   if (filename == NULL || filename[0] == 0) {
      filename = "vault";
   }
   if (filename[0] == '/') {
      path = szprintf(memory, NULL, "%s", filename);
   } else {
      read_t read = read_xdg_file_from_dirs(memory, filename, xdg_data_dirs());
      path = read.path;
      if (read.file != NULL) {
         int n = fclose(read.file);
         assert(n == 0);
      }
   }
   log_info(log, "Vault path is %s", path);
   result->database = db_factory(memory, log, path);
   memory.free(path);

   return I(result);
}
