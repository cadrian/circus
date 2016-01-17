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

#include <cad_hash.h>
#include <gcrypt.h>
#include <json.h>
#include <string.h>

#include "circus.h"
#include "log.h"
#include "vault.h"
#include "xdg.h"

#define SECURE_MEM_SIZE 65536

#define gcrypt(fn) do {                                                 \
      gcry_error_t e ## __LINE__ = gcry_ ## fn;                         \
      if (e ## __LINE__) {                                              \
         log_error(LOG, "vault", "%s:%d - %s/%s", __FILE__, __LINE__,   \
                   gcry_strsource(e ## __LINE__),                       \
                   gcry_strerror(e ## __LINE__));                       \
         exit(1);                                                       \
      }                                                                 \
   } while(0)

extern circus_log_t *LOG;

typedef struct {
   circus_vault_t fn;
   cad_memory_t memory;
   json_object_t *vault;
   cad_hash_t *keys;
} vault_impl_t;

typedef struct {
   circus_key_t fn;
   cad_memory_t memory;
   json_object_t *key;
   vault_impl_t *vault;
} key_impl_t;

/* ----------------------------------------------------------------
 * JSON object type checker
 * ---------------------------------------------------------------- */

typedef struct {
   json_visitor_t fn;
   json_object_t *object;
   json_array_t *array;
   json_string_t *string;
   json_number_t *number;
   json_const_t *constant;
} vault_read_checker;

static void vault_read_checker_free(vault_read_checker *this) {
   // never allocated
   UNUSED(this);
}

static void vault_read_checker_visit_object(vault_read_checker *this, json_object_t *visited) {
   this->object = visited;
}

static void vault_read_checker_visit_array(vault_read_checker *this, json_array_t *visited) {
   this->array = visited;
}

static void vault_read_checker_visit_string(vault_read_checker *this, json_string_t *visited) {
   this->string = visited;
}

static void vault_read_checker_visit_number(vault_read_checker *this, json_number_t *visited) {
   this->number = visited;
}

static void vault_read_checker_visit_const(vault_read_checker *this, json_const_t *visited) {
   this->constant = visited;
}

static json_visitor_t vault_read_checker_fn = {
   (json_visit_free_fn)vault_read_checker_free,
   (json_visit_object_fn)vault_read_checker_visit_object,
   (json_visit_array_fn)vault_read_checker_visit_array,
   (json_visit_string_fn)vault_read_checker_visit_string,
   (json_visit_number_fn)vault_read_checker_visit_number,
   (json_visit_const_fn)vault_read_checker_visit_const,
};

static vault_read_checker check_json_value(json_value_t *data) {
   vault_read_checker checker = { vault_read_checker_fn, NULL, NULL, NULL, NULL, NULL };
   data->accept(data, (json_visitor_t*)&checker);
   return checker;
}

/* ----------------------------------------------------------------
 * Key implementation
 * ---------------------------------------------------------------- */

static char *key_get_password(key_impl_t *this, const char *enc_key) {
   vault_read_checker checker;
   json_string_t *salt, *password;
   char *key, *pwd, *b64pwd;
   char mac_key[32];
   size_t keylen, pwdlen, b64pwdlen, reslen;
   char *result;

   assert(enc_key != NULL);

   checker = check_json_value(this->key->get(this->key, "salt"));
   salt = checker.string;
   assert(salt != NULL);

   checker = check_json_value(this->key->get(this->key, "password"));
   password = checker.string;;
   assert(password != NULL);

   keylen = salt->utf8(salt, "", 0) + strlen(enc_key) + 1;
   key = this->memory.malloc(keylen);
   salt->utf8(salt, key, keylen);
   strcat(key, enc_key);

   b64pwdlen = password->utf8(password, "", 0) + 1;
   b64pwd = this->memory.malloc(b64pwdlen);
   password->utf8(password, b64pwd, b64pwdlen);

   // TODO b64 -> bin
   pwd = NULL;
   pwdlen = 0;

   gcry_cipher_hd_t h;
   gcrypt(cipher_open(&h, GCRY_CIPHER_AES256, GCRY_CIPHER_MODE_CFB, GCRY_CIPHER_SECURE));
   gcrypt(cipher_setkey(h, mac_key, 32));
   if (gcry_cipher_decrypt(h, "", 0, pwd, pwdlen)) {
      // TODO malloc result
      result = NULL;
      reslen = 0;
      gcrypt(cipher_decrypt(h, result, reslen, pwd, pwdlen));
   }
   gcry_cipher_close(h);

   memset(key, 0, keylen);
   this->memory.free(key);
   memset(pwd, 0, pwdlen);
   this->memory.free(pwd);

   return result;
}

static void key_set_password(key_impl_t *this, const char *enc_key, const char *buffer) {
   // TODO
   UNUSED(this);
   UNUSED(enc_key);
   UNUSED(buffer);
}

static json_object_t *key_properties(key_impl_t *this) {
   // TODO
   UNUSED(this);
   return NULL;
}

static void key_free(key_impl_t *this) {
   this->memory.free(this); // don't free this->key, it belongs to the vault's JSON object
}

static circus_key_t key_fn = {
   (circus_key_get_password_fn)key_get_password,
   (circus_key_set_password_fn)key_set_password,
   (circus_key_properties_fn)key_properties,
   (circus_key_free_fn)key_free,
};

/* ----------------------------------------------------------------
 * Vault implementation
 * ---------------------------------------------------------------- */

static key_impl_t *vault_get(vault_impl_t *this, const char *name) {
   key_impl_t *result = this->keys->get(this->keys, name);
   if (result == NULL) {
      json_value_t *data = this->vault->get(this->vault, name);
      if (data != NULL) {
         vault_read_checker checker = check_json_value(data);
         if (checker.object == NULL) {
            fprintf(stderr, "**** Invalid vault");
            exit(1);
         }
         result = this->memory.malloc(sizeof(key_impl_t));
         result->fn = key_fn;
         result->memory = this->memory;
         result->key = checker.object;
         result->vault = this;
         this->keys->set(this->keys, name, result);
      }
   }
   return result;
}

static key_impl_t *vault_new(vault_impl_t *this, const char *name) {
   assert(vault_get(this, name) == NULL);

   key_impl_t *result = this->memory.malloc(sizeof(key_impl_t));
   result->fn = key_fn;
   result->memory = this->memory;
   result->key = json_new_object(this->memory);
   result->vault = this;

   result->key->set(result->key, "salt", (json_value_t*)json_new_string(this->memory));
   result->key->set(result->key, "password", (json_value_t*)json_new_string(this->memory));
   result->key->set(result->key, "properties", (json_value_t*)json_new_object(this->memory));

   this->vault->set(this->vault, name, (json_value_t*)result->key);
   this->keys->set(this->keys, name, result);

   return result;
}

static void vault_clean(cad_hash_t *hash, int index, const char *name, circus_key_t *key, vault_impl_t *vault) {
   UNUSED(hash);
   UNUSED(index);
   UNUSED(name);
   UNUSED(vault);
   key->free(key);
}

static void vault_free(vault_impl_t *this) {
   this->keys->clean(this->keys, (cad_hash_iterator_fn)vault_clean, this);
   this->keys->free(this->keys);
   this->vault->free(this->vault);
   this->memory.free(this);
}

static circus_vault_t vault_fn = {
   (circus_vault_get_fn)vault_get,
   (circus_vault_new_fn)vault_new,
   (circus_vault_free_fn)vault_free,
};

static void gcrypt_log_handler(void *data, int level, const char *msg) {
   assert(data == NULL);
   switch (level) {
   case GCRY_LOG_CONT:
   case GCRY_LOG_INFO:
      log_info(LOG, "vault", "gcrypt: %s", msg);
      break;
   case GCRY_LOG_WARN:
      log_warning(LOG, "vault", "gcrypt: %s", msg);
      break;
   case GCRY_LOG_ERROR:
   case GCRY_LOG_FATAL:
   case GCRY_LOG_BUG:
      log_error(LOG, "vault", "gcrypt: %s", msg);
      break;
   case GCRY_LOG_DEBUG:
      log_debug(LOG, "vault", "gcrypt: %s", msg);
      break;
   default:
      log_error(LOG, "vault", "gcrypt: [?%d?] %s", level, msg);
   }
}

static void gcrypt_error_handler(void *data, int level, const char *msg) {
   gcrypt_log_handler(data, level, msg);
   crash();
}

static void gcrypt_init(void) {
   int err;
   const char *ver;

   gcry_set_fatalerror_handler((gcry_handler_error_t)gcrypt_error_handler, NULL);
   gcry_set_log_handler((gcry_handler_log_t)gcrypt_log_handler, NULL);

   err = gcry_control(GCRYCTL_SET_VERBOSITY, 2);
   assert(err == 0);
   err = gcry_control(GCRYCTL_SET_PREFERRED_RNG_TYPE, GCRY_RNG_TYPE_SYSTEM);
   assert(err == 0);

   ver = gcry_check_version(GCRYPT_VERSION);
   if (ver == NULL) {
      log_error(LOG, "vault", "incompatible gcrypt version, %s or later required", GCRYPT_VERSION);
      exit(1);
   }
   //err = gcry_control(GCRYCTL_SUSPEND_SECMEM_WARN);
   //assert(err == 0);
   err = gcry_control(GCRYCTL_USE_SECURE_RNDPOOL);
   assert(err == 0);
   err = gcry_control(GCRYCTL_INIT_SECMEM, SECURE_MEM_SIZE);
   assert(err == 0);
   //err = gcry_control(GCRYCTL_RESUME_SECMEM_WARN);
   //assert(err == 0);
   err = gcry_control(GCRYCTL_INITIALIZATION_FINISHED);
   assert(err == 0);
}

static void vault_error(cad_input_stream_t *stream, int line, int column, void *data, const char *format, ...) {
   UNUSED(stream);
   vault_impl_t *this = (vault_impl_t*)data;
   va_list args;
   char *log;
   va_start(args, format);
   log = vszprintf(this->memory, NULL, format, args);
   va_end(args);
   log_error(LOG, "vault", "Error while reading vault file, line %d, column %d: %s", line, column, log);
   this->memory.free(log);
}

circus_vault_t *circus_vault(cad_memory_t memory, const char *enc_key, const char *filename) {
   UNUSED(enc_key); // TODO
   static int init = 0;
   vault_impl_t *result;
   FILE *file;
   char *path;
   int n;
   cad_input_stream_t *raw, *stream;
   json_value_t *data;

   if (!init) {
      init = 1;
      gcrypt_init();
   }

   if (!gcry_control(GCRYCTL_INITIALIZATION_FINISHED_P)) {
      log_error(LOG, "vault", "libgcrypt initialization failed");
      exit(1);
   }

   result = memory.malloc(sizeof(vault_impl_t));
   assert(result != NULL);

   path = szprintf(memory, NULL, "%s/%s", xdg_data_home(), filename);
   file = fopen(path, "r");
   if (file == NULL) {
      result->vault = json_new_object(memory);
   } else {
      raw = new_cad_input_stream_from_file(file, memory);
      assert(raw != NULL);
      stream = new_json_utf8_stream(raw, memory);
      assert(stream != NULL);

      data = json_parse(stream, vault_error, result, memory);
      if (data == NULL) {
         log_error(LOG, "vault", "JSON parse error in file %s", path);
         exit(1);
      }

      vault_read_checker checker = check_json_value(data);
      if (checker.object == NULL) {
         fprintf(stderr, "**** Invalid vault");
         exit(1);
      }
      result->vault = checker.object;

      stream->free(stream);
      raw->free(raw);
      n = fclose(file);
      assert(n == 0);
   }
   memory.free(path);

   result->fn = vault_fn;
   result->memory = memory;

   return (circus_vault_t*)result;
}
