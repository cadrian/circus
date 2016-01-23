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
#ifndef __CIRCUS_VAULT_H
#define __CIRCUS_VAULT_H

#include <json.h>

typedef struct circus_key_s circus_key_t;

typedef char *(*circus_key_get_password_fn)(circus_key_t *this, const char *enc_key);
typedef void (*circus_key_set_password_fn)(circus_key_t *this, const char *enc_key, const char *buffer);
typedef json_object_t* (*circus_key_properties_fn)(circus_key_t *this);
typedef void (*circus_key_free_fn)(circus_key_t *this);

struct circus_key_s {
   circus_key_get_password_fn get_password;
   circus_key_set_password_fn set_password;
   circus_key_properties_fn properties;
   circus_key_free_fn free;
};

typedef struct circus_vault_s circus_vault_t;

typedef circus_key_t *(*circus_vault_get_fn)(circus_vault_t *this, const char *name);
typedef circus_key_t *(*circus_vault_new_fn)(circus_vault_t *this, const char *name);
typedef void (*circus_vault_free_fn)(circus_vault_t *this);

struct circus_vault_s {
   circus_vault_get_fn get;
   circus_vault_new_fn new;
   circus_vault_free_fn free;
};

__PUBLIC__ circus_vault_t *circus_vault(cad_memory_t memory, const char *enc_key, const char *filename);

#endif /* __CIRCUS_VAULT_H */
