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

#include <gcrypt.h>
#include <string.h>

#include <circus_base64.h>
#include <circus_crypt.h>

#define SALT_SIZE 16
#define KEY_SIZE 32 // 256 bits
#define HASH_SIZE 64 // 512 bits
#define SECURE_MEM_SIZE 65536

#define gcrypt(fn) ({                                                   \
         gcry_error_t e ## __LINE__ = gcry_ ## fn;                      \
         if (e ## __LINE__) {                                           \
            log_error(log, "vault_enc", "%s:%d - %s/%s",                \
                      __FILE__, __LINE__,                               \
                      gcry_strsource(e ## __LINE__),                    \
                      gcry_strerror(e ## __LINE__));                    \
         }                                                              \
         e ## __LINE__;                                                 \
      })

char *salt(cad_memory_t memory, circus_log_t *log) {
   char *result = NULL;
   char *raw = memory.malloc(SALT_SIZE);
   if (raw == NULL) {
      log_error(log, "vault_enc", "Could not allocate memory for salt");
   } else {
      gcry_randomize(raw, SALT_SIZE, GCRY_STRONG_RANDOM);
      result = base64(memory, raw, SALT_SIZE);
      memory.free(raw);
   }
   return result;
}

char *salted(cad_memory_t memory, circus_log_t *log, const char *salt, const char *value) {
   assert(strlen(salt) == b64_size(SALT_SIZE));
   assert(value != NULL);
   assert(value[0] != 0);
   char *result = szprintf(memory, NULL, "%s:%s", salt, value);
   if (result == NULL) {
      log_error(log, "vault_enc", "Could not allocate memory for salted");
   }
   return result;
}

char *unsalted(cad_memory_t memory, circus_log_t *log, const char *salt, const char *value) {
   assert(strlen(salt) == SALT_SIZE);
   assert(value != NULL);
   assert(value[0] != 0);

   char *result = NULL;
   int n = strlen(value) - SALT_SIZE; /* - 1 (for ':') + 1 (for '\0') */

   if (n > 1 && value[SALT_SIZE] == ':' && memcmp(value, salt, SALT_SIZE) == 0) {
      result = memory.malloc(n);
      memcpy(result, value + SALT_SIZE + 1, n);
   } else {
      log_error(log, "vault_enc", "Tampered salted value!!");
   }

   return result;
}

char *hashed(cad_memory_t memory, circus_log_t *log, const char *value) {
   assert(value != NULL);
   assert(value[0] != 0);

   gcry_md_hd_t hd;
   gcry_error_t e = gcrypt(md_open(&hd, GCRY_MD_SHA512, GCRY_MD_FLAG_SECURE));
   if (e != 0) {
      log_error(log, "vault_enc", "Could not open hash algorithm");
      return NULL;
   }
   gcry_md_write(hd, value, strlen(value));
   gcry_md_final(hd);

   char *result = NULL;
   char *hash = (char*)gcry_md_read(hd, 0);
   if (hash != NULL) {
      result = base64(memory, hash, HASH_SIZE);
   }
   gcry_md_close(hd);

   return result;
}

char *new_symmetric_key(cad_memory_t memory, circus_log_t *log) {
   char *raw = memory.malloc(KEY_SIZE);
   if (raw == NULL) {
      log_error(log, "vault_enc", "Could not allocate memory for symmetric key");
   } else {
      gcry_randomize(raw, KEY_SIZE, GCRY_VERY_STRONG_RANDOM);
   }
   char *result = base64(memory, raw, KEY_SIZE);
   memory.free(raw);
   return result;
}

char *encrypted(cad_memory_t memory, circus_log_t *log, const char *value, const char *b64key) {
   assert(value != NULL);
   assert(value[0] != 0);

   char *result = NULL;
   char *key;
   gcry_cipher_hd_t hd;
   gcry_error_t e = gcrypt(cipher_open(&hd, GCRY_CIPHER_AES256, GCRY_CIPHER_MODE_CFB, GCRY_CIPHER_SECURE));
   if (e != 0) {
      return NULL;
   }
   int len = strlen(value);
   int n = len + KEY_SIZE - (len % KEY_SIZE);
   char *raw = memory.malloc(n);
   memcpy(raw, value, len);
   key = unbase64(memory, b64key);
   if (key != NULL) {
      e = gcrypt(cipher_setkey(hd, key, KEY_SIZE));
      if (e == 0) {
         e = gcrypt(cipher_encrypt(hd, raw, n, NULL, 0));
         if (e == 0) {
            result = base64(memory, raw, n);
         }
      }
      memory.free(key); // TODO try to free it earlier (to be tested)
   }
   memory.free(raw);
   gcry_cipher_close(hd);

   return result;
}

char *decrypted(cad_memory_t memory, circus_log_t *log, const char *b64value, const char *b64key) {
   assert(b64value != NULL);
   assert(b64value[0] != 0);

   char *value = unbase64(memory, b64value);
   if (value == NULL) {
      return NULL;
   }

   char *result = NULL;
   char *key;
   gcry_cipher_hd_t hd;
   gcry_error_t e = gcrypt(cipher_open(&hd, GCRY_CIPHER_AES256, GCRY_CIPHER_MODE_CFB, GCRY_CIPHER_SECURE));
   if (e != 0) {
      return NULL;
   }
   int len = ((strlen(b64value) + 3) / 4) * 3;
   assert(len % KEY_SIZE == 0);
   key = unbase64(memory, b64key);
   if (key != NULL) {
      e = gcrypt(cipher_setkey(hd, key, KEY_SIZE));
      if (e == 0) {
         e = gcrypt(cipher_decrypt(hd, value, len, NULL, 0));
         if (e == 0) {
            result = value;
         }
      }
      memory.free(key); // TODO try to free it earlier (to be tested)
   }
   gcry_cipher_close(hd);

   if (result != value) {
      memory.free(value);
   }

   return result;
}

static char *szrandom_level(cad_memory_t memory, size_t len, enum gcry_random_level level) {
   assert(len > 0);
   char *raw = memory.malloc(len + 1);
   gcry_randomize(raw, len, level);
   char *result = base64(memory, raw, len);
   memory.free(raw);
   return result;
}

char *szrandom(cad_memory_t memory, size_t len) {
   return szrandom_level(memory, len, GCRY_STRONG_RANDOM);
}

char *szrandom_strong(cad_memory_t memory, size_t len) {
   return szrandom_level(memory, len, GCRY_VERY_STRONG_RANDOM);
}

void __wrap_gcry_randomize(unsigned char *buffer, size_t length, enum gcry_random_level UNUSED(level)) {
   static unsigned char c = 0;
   for (size_t i = 0; i < length; i++) {
      buffer[i] = c++;
   }
}
