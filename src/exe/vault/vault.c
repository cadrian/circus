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
#include <sys/stat.h>
#include <sys/types.h>

#include <circus.h>
#include <circus_crypt.h>
#include <circus_log.h>
#include <circus_vault.h>
#include <circus_xdg.h>

#include "vault_impl.h"

static void get_symmetric_key(user_impl_t *user, const char *password) {
   static const char *sql = "SELECT KEYSALT, HASHKEY FROM USERS WHERE USERID=?";
   sqlite3_stmt *stmt;
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
            const char *keysalt = (const char*)sqlite3_column_text(stmt, 0);
            const char *hashkey = (const char*)sqlite3_column_text(stmt, 1);

            if (hashkey == NULL || strlen(hashkey) == 0) {
               log_error(user->log, "Error user: %ld -- HASHKEY is NULL", (long int)user->userid);
            } else if (keysalt == NULL || strlen(keysalt) == 0) {
               log_error(user->log, "Error user: %ld -- KEYSALT is NULL", (long int)user->userid);
            } else {
               char *passslt=NULL;
               char *passkey=NULL;
               passslt = salted(user->memory, user->log, keysalt, password);
               if (passslt == NULL) {
                  log_error(user->log, "Error user: %ld -- could not salt password", (long int)user->userid);
               } else {
                  passkey = hashed(user->memory, user->log, passslt);
                  if (passkey == NULL) {
                     log_error(user->log, "Error user: %ld -- could not hash password", (long int)user->userid);
                  } else {
                     char *saltedkey = decrypted(user->memory, user->log, hashkey, passkey);
                     if (saltedkey == NULL) {
                        log_error(user->log, "Error user: %ld -- could not decrypt", (long int)user->userid);
                     } else {
                        user->symmkey = unsalted(user->memory, user->log, keysalt, saltedkey);
                        if (user->symmkey == NULL) {
                           log_error(user->log, "Error user: %ld -- could not unsalt", (long int)user->userid);
                        }
                        user->memory.free(saltedkey);
                     }
                     user->memory.free(passkey);
                  }
                  user->memory.free(passslt);
               }
            }
         }
      }
      n = sqlite3_finalize(stmt);
      if (n != SQLITE_OK) {
         log_warning(user->log, "Error in finalize: %s", sqlite3_errstr(n));
      }
   }
}

int set_symmetric_key(user_impl_t *user, const char *password) {
   int result = 0;
   static const char *sql = "UPDATE USERS SET KEYSALT=?, HASHKEY=? WHERE USERID=?";
   sqlite3_stmt *stmt;
   int n = sqlite3_prepare_v2(user->vault->db, sql, -1, &stmt, NULL);
   if (n != SQLITE_OK) {
      log_error(user->log, "Error preparing statement: %s -- %s", sql, sqlite3_errstr(n));
   } else {
      int ok=1;
      char *keysalt=NULL;
      if (ok) {
         keysalt = salt(user->memory, user->log);
         if (keysalt == NULL) {
            ok=0;
         } else {
            n = sqlite3_bind_text(stmt, 1, keysalt, -1, SQLITE_TRANSIENT);
            if (n != SQLITE_OK) {
               log_error(user->log, "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
               ok=0;
            }
         }
      }

      char *hashkey=NULL;
      char *enckey=NULL;
      char *sltkey=NULL;
      char *passslt=NULL;
      char *passkey=NULL;
      if (ok) {
         if (user->symmkey == NULL) {
            user->symmkey = new_symmetric_key(user->memory, user->log);
         }
         enckey = szprintf(user->memory, NULL, "%s", user->symmkey);
         if (enckey == NULL) {
            ok=0;
         } else {
            sltkey = salted(user->memory, user->log, keysalt, enckey);
            if (sltkey == NULL) {
               ok=0;
            } else {
               passslt = salted(user->memory, user->log, keysalt, password);
               if (passslt == NULL) {
                  ok=0;
               } else {
                  passkey = hashed(user->memory, user->log, passslt);
                  if (passkey == NULL) {
                     ok=0;
                  } else {
                     hashkey = encrypted(user->memory, user->log, sltkey, passkey);
                     if (hashkey == NULL) {
                        ok=0;
                     } else {
                        n = sqlite3_bind_text(stmt, 2, hashkey, -1, SQLITE_TRANSIENT);
                        if (n != SQLITE_OK) {
                           log_error(user->log, "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
                           ok=0;
                        }
                     }
                  }
               }
            }
         }
      }

      if (ok) {
         n = sqlite3_bind_int64(stmt, 3, user->userid);
         if (n != SQLITE_OK) {
            log_error(user->log, "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
            ok=0;
         }
      }

      if (ok) {
         while ((n = sqlite3_step(stmt)) == SQLITE_ROW) {
         }
         if (n == SQLITE_OK || n == SQLITE_DONE) {
            result = 1;
         } else {
            log_error(user->log, "Error stepping statement: %s -- %s", sql, sqlite3_errstr(n));
         }
      }

      user->memory.free(sltkey);
      user->memory.free(enckey);
      user->memory.free(hashkey);
      user->memory.free(keysalt);
      user->memory.free(passslt);
      user->memory.free(passkey);

      n = sqlite3_finalize(stmt);
      if (n != SQLITE_OK) {
         log_warning(user->log, "Error in finalize: %s", sqlite3_errstr(n));
      }
   }

   return result;
}

static user_impl_t *vault_get_(vault_impl_t *this, const char *username, const char *password, int with_symmkey) {
   log_info(this->log, "Getting user %s%s%s", username, with_symmkey ? " with their symmetric key" : "", password == NULL ? "" : ", and checking password");

   user_impl_t *result = this->users->get(this->users, username);
   if (result == NULL) {
      log_debug(this->log, "User %s not found in dict, loading from database", username);
      static const char *sql = "SELECT USERID, PERMISSIONS, EMAIL, PWDVALID FROM USERS WHERE USERNAME=?";
      sqlite3_stmt *stmt;
      int n = sqlite3_prepare_v2(this->db, sql, -1, &stmt, NULL);
      if (n != SQLITE_OK) {
         log_error(this->log, "Error preparing statement: %s -- %s", sql, sqlite3_errstr(n));
      } else {
         n = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
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
                     log_error(this->log, "Error: multiple entries for user %s", username);
                     result->fn.free(&(result->fn));
                     result = NULL;
                     done = 1;
                  } else {
                     sqlite3_int64 userid = sqlite3_column_int64(stmt, 0);
                     int permissions = (int)sqlite3_column_int64(stmt, 1);
                     const char *email = (const char*)sqlite3_column_text(stmt, 2);
                     sqlite3_int64 validity = sqlite3_column_int64(stmt, 3);
                     result = new_vault_user(this->memory, this->log, userid, validity, permissions, email, username, this);
                  }
                  break;
               case SQLITE_DONE:
                  done = 1;
                  break;
               default:
                  log_error(this->log, "Error stepping statement: %s -- %d: %s", sql, n, sqlite3_errstr(n));
                  done = 1;
               }
            } while (!done);
         }
      }
      n = sqlite3_finalize(stmt);
      if (n != SQLITE_OK) {
         log_warning(this->log, "Error in finalize: %s", sqlite3_errstr(n));
      }
      if (result != NULL) {
         log_debug(this->log, "Updating users cache");
         this->users->set(this->users, username, result);
         if (with_symmkey) {
            log_debug(this->log, "Getting user symmetric key");
            get_symmetric_key(result, password);
         }
      }
   }

   if (result == NULL) {
      log_error(this->log, "Could not find user %s", username);
   } else {
      log_pii(this->log, "User %s is user %ld", username, (long int)result->userid);
      if (password != NULL) {
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
   log_info(this->log, "Creating new user %s", username);

   user_impl_t *result = NULL;
   static const char *sql = "INSERT INTO USERS (USERNAME, PERMISSIONS, PWDSALT, HASHPWD, PWDVALID, KEYSALT, HASHKEY) values (?, ?, ?, ?, ?, 'invalid', 'invalid')";
   sqlite3_stmt *stmt;
   int n = sqlite3_prepare_v2(this->db, sql, -1, &stmt, NULL);
   if (n != SQLITE_OK) {
      log_error(this->log, "Error preparing statement: %s -- %s", sql, sqlite3_errstr(n));
   } else {
      int ok=1;
      if (ok) {
         n = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
         if (n != SQLITE_OK) {
            log_error(this->log, "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
            ok=0;
         }
      }
      if (ok) {
         n = sqlite3_bind_int64(stmt, 2, (sqlite3_int64)permissions);
         if (n != SQLITE_OK) {
            log_error(this->log, "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
            ok=0;
         }
      }
      char *pwdsalt=NULL;
      if (ok) {
         pwdsalt = salt(this->memory, this->log);
         if (pwdsalt == NULL) {
            log_error(this->log, "Error creating salt");
            ok=0;
         } else {
            n = sqlite3_bind_text(stmt, 3, pwdsalt, -1, SQLITE_TRANSIENT);
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
            log_error(this->log, "Error salting password");
            ok=0;
         } else {
            hashpwd = hashed(this->memory, this->log, pwd);
            if (hashpwd == NULL) {
               log_error(this->log, "Error hashing password");
               ok=0;
            } else {
               n = sqlite3_bind_text(stmt, 4, hashpwd, -1, SQLITE_TRANSIENT);
               if (n != SQLITE_OK) {
                  log_error(this->log, "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
                  ok=0;
               }
            }
         }
      }
      if (ok) {
         n = sqlite3_bind_int64(stmt, 5, (sqlite3_int64)validity);
         if (n != SQLITE_OK) {
            log_error(this->log, "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
            ok=0;
         }
      }

      if (ok) {
         while ((n = sqlite3_step(stmt)) == SQLITE_ROW) {
         }
         if (n != SQLITE_OK && n != SQLITE_DONE) {
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

      if (ok) {
         result = vault_get_(this, username, password, 0);
         if (result == NULL) {
            log_error(this->log, "Error getting the just-created user from the database");
            ok=0;
         } else {
            log_debug(this->log, "Creating user symmetric key");
            ok=set_symmetric_key(result, password);
            if (!ok) {
               log_error(this->log, "Symmetric key encryption creation failed. The user %ld may need help.", (long int)result->userid);
            }
         }
      }
   }

   return result;
}

static user_impl_t *vault_new(vault_impl_t *this, const char *username, const char *password, uint64_t validity) {
   assert(vault_get_(this, username, password, 0) == NULL);
   return vault_new_(this, username, password, validity, PERMISSION_USER);
}

static int vault_install(vault_impl_t *this, const char *admin_username, const char *admin_password) {
   int status = 0;
   int n;
   char *err;

   n = sqlite3_exec(this->db, META_SCHEMA, NULL, NULL, &err);
   if (n != SQLITE_OK) {
      log_error(this->log, "Error creating META table: %s", err);
      sqlite3_free(err);
      sqlite3_close(this->db);
      status = 1;
   }

   n = sqlite3_exec(this->db, USERS_SCHEMA, NULL, NULL, &err);
   if (n != SQLITE_OK) {
      log_error(this->log, "Error creating USERS table: %s", err);
      sqlite3_free(err);
      sqlite3_close(this->db);
      status = 1;
   }

   n = sqlite3_exec(this->db, KEYS_SCHEMA, NULL, NULL, &err);
   if (n != SQLITE_OK) {
      log_error(this->log, "Error creating KEYS table: %s", err);
      sqlite3_free(err);
      sqlite3_close(this->db);
      status = 1;
   }

   n = sqlite3_exec(this->db, TAGS_SCHEMA, NULL, NULL, &err);
   if (n != SQLITE_OK) {
      log_error(this->log, "Error creating TAGS table: %s", err);
      sqlite3_free(err);
      sqlite3_close(this->db);
      status = 1;
   }

   static const char *sql = "INSERT INTO META (KEY, VALUE) VALUES ('VERSION', ?)";
   sqlite3_stmt *stmt;
   n = sqlite3_prepare_v2(this->db, sql, -1, &stmt, NULL);
   if (n != SQLITE_OK) {
      log_error(this->log, "Error preparing statement: %s -- %s", sql, sqlite3_errstr(n));
      status = 1;
   } else {
      n = sqlite3_bind_text(stmt, 1, DB_VERSION, -1, SQLITE_STATIC);
      if (n != SQLITE_OK) {
         log_error(this->log, "Error binding statement: %s -- %s", sql, sqlite3_errstr(n));
         status = 1;
      } else {
         while ((n = sqlite3_step(stmt)) == SQLITE_ROW) {
         }
         if (n != SQLITE_OK && n != SQLITE_DONE) {
            log_error(this->log, "Error stepping statement: %s -- %s", sql, sqlite3_errstr(n));
            status = 1;
         }
      }
      n = sqlite3_finalize(stmt);
      if (n != SQLITE_OK) {
         log_warning(this->log, "Error in finalize: %s", sqlite3_errstr(n));
      }
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
   sqlite3_close(this->db);
   this->memory.free(this);
}

static circus_vault_t vault_fn = {
   (circus_vault_get_fn)vault_get,
   (circus_vault_new_fn)vault_new,
   (circus_vault_install_fn)vault_install,
   (circus_vault_free_fn)vault_free,
};

static void mkparentdirs(cad_memory_t memory, const char *dir) {
   char *tmp;
   char *p = NULL;
   int len, i;

   tmp = szprintf(memory, &len, "%s", dir);
   assert(tmp != NULL);
   if (tmp[len - 1] == '/') {
      tmp[--len] = 0;
   }
   for (i = len - 1; i > 0 && tmp[i] != '/'; i--) {
      // just looping to find the last '/', to remove the filename
   }
   assert(i > 0);
   assert(tmp[i] == '/');
   tmp[i] = 0;

   for (p = tmp + 1; *p; p++) {
      if (*p == '/') {
         *p = 0;
         mkdir(tmp, 0770);
         *p = '/';
      }
   }
   mkdir(tmp, 0700);

   memory.free(tmp);
}


circus_vault_t *circus_vault(cad_memory_t memory, circus_log_t *log, circus_config_t *config) {
   vault_impl_t *result = NULL;
   char *path;
   int n;
   const char *filename = config->get(config, "vault", "filename");
   if (filename == NULL || filename[0] == 0) {
      filename = "vault";
   }
   if (filename[0] == '/') {
      path = szprintf(memory, NULL, "%s", filename);
   } else {
      path = szprintf(memory, NULL, "%s/%s", xdg_data_home(), filename);
   }

   result = memory.malloc(sizeof(vault_impl_t));
   assert(result != NULL);
   result->fn = vault_fn;
   result->memory = memory;
   result->log = log;
   result->users = cad_new_hash(memory, cad_hash_strings);

   mkparentdirs(memory, path);
   n = sqlite3_open_v2(path, &(result->db),
                       SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_PRIVATECACHE,
                       "unix-excl");
   if (n != SQLITE_OK) {
      log_error(log, "Cannot open database: %s -- %s", path, sqlite3_errmsg(result->db));
      sqlite3_close(result->db);
      memory.free(result);
      return NULL;
   }
   memory.free(path);

   return I(result);
}
