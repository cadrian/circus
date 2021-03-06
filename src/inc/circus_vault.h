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

#ifndef __CIRCUS_VAULT_H
#define __CIRCUS_VAULT_H

#include <json.h>
#include <stdint.h>
#include <time.h>

#include <cad_hash.h>

#include <circus.h>
#include <circus_config.h>
#include <circus_database.h>
#include <circus_log.h>

typedef struct circus_key_s circus_key_t;

typedef char *(*circus_key_get_password_fn)(circus_key_t *this);
typedef int (*circus_key_set_password_fn)(circus_key_t *this, const char *password);
typedef void (*circus_key_free_fn)(circus_key_t *this);

struct circus_key_s {
   circus_key_get_password_fn get_password;
   circus_key_set_password_fn set_password;
   circus_key_free_fn free;
};

typedef struct circus_user_s circus_user_t;

typedef circus_key_t *(*circus_user_get_fn)(circus_user_t *this, const char *keyname);
typedef cad_hash_t *(*circus_user_get_all_fn)(circus_user_t *this);
typedef circus_key_t *(*circus_user_new_fn)(circus_user_t *this, const char *keyname);
typedef const char *(*circus_user_name_fn)(circus_user_t *this);
typedef int (*circus_user_set_password_fn)(circus_user_t *this, const char *password, uint64_t validity);
typedef int (*circus_user_set_email_fn)(circus_user_t *this, const char *email);
typedef int (*circus_user_is_admin_fn)(circus_user_t *this);
typedef time_t (*circus_user_validity_fn)(circus_user_t *this);
typedef void (*circus_user_free_fn)(circus_user_t *this);

struct circus_user_s {
   circus_user_get_fn get;
   circus_user_get_all_fn get_all;
   circus_user_new_fn new;
   circus_user_name_fn name;
   circus_user_set_password_fn set_password;
   circus_user_set_email_fn set_email;
   circus_user_is_admin_fn is_admin;
   circus_user_validity_fn validity;
   circus_user_free_fn free;
};

typedef struct circus_vault_s circus_vault_t;

typedef circus_user_t *(*circus_vault_get_fn)(circus_vault_t *this, const char *username, const char *password);
typedef circus_user_t *(*circus_vault_new_fn)(circus_vault_t *this, const char *username, const char *password, uint64_t validity);
typedef int (*circus_vault_install_fn)(circus_vault_t *this, const char *admin_username, const char *admin_password);
typedef void (*circus_vault_free_fn)(circus_vault_t *this);

struct circus_vault_s {
   circus_vault_get_fn get;
   circus_vault_new_fn new;
   circus_vault_install_fn install;
   circus_vault_free_fn free;
};

typedef circus_database_t *(*database_factory_fn)(cad_memory_t memory, circus_log_t *log, const char *path);
__PUBLIC__ circus_vault_t *circus_vault(cad_memory_t memory, circus_log_t *log, circus_config_t *config, database_factory_fn db_factory);

#endif /* __CIRCUS_VAULT_H */
