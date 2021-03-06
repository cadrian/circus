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

#include <gcrypt.h>
#include <string.h>

#include <circus_base32.h>
#include <circus_base64.h>
#include <circus_crypt.h>

#define SALT_SIZE 16
#define KEY_SIZE 32 // 256 bits
#define HASH_SIZE 32 // 256 bits
// BEWARE!! KEY_SIZE == HASH_SIZE otherwise some bits are lost or uninitialized

#define gcrypt(fn) ({                                      \
         gcry_error_t e ## __LINE__ = gcry_ ## fn;         \
         if (e ## __LINE__) {                              \
            logger_error(log, __FILE__, __LINE__, "%s/%s", \
                         gcry_strsource(e ## __LINE__),    \
                         gcry_strerror(e ## __LINE__));    \
         }                                                 \
         e ## __LINE__;                                    \
      })

char *salt(cad_memory_t memory, circus_log_t *log) {
   char *result = NULL;
   char *raw = memory.malloc(SALT_SIZE);
   if (raw == NULL) {
      log_error(log, "Could not allocate memory for salt");
   } else {
      gcry_randomize(raw, SALT_SIZE, GCRY_STRONG_RANDOM);
      result = base64(memory, raw, SALT_SIZE);
      memory.free(raw);
   }
   return result;
}

char *salted(cad_memory_t memory, circus_log_t *log, const char *salt, const char *value) {
   //assert(strlen(salt) == b64_size(SALT_SIZE));
   assert(strlen(salt) > 0);
   assert(value != NULL);
   assert(value[0] != 0);
   char *result = szprintf(memory, NULL, "%s:%s", salt, value);
   if (result == NULL) {
      log_error(log, "Could not allocate memory for salted");
   }
   return result;
}

char *unsalted(cad_memory_t memory, circus_log_t *log, const char *salt, const char *value) {
   //assert(strlen(salt) == b64_size(SALT_SIZE));
   assert(strlen(salt) > 0);
   assert(value != NULL);
   assert(value[0] != 0);

   char *result = NULL;
   int saltlen = strlen(salt);
   int n = strlen(value) - saltlen; /* - 1 (for ':') + 1 (for '\0') */

   if (n > 1 && value[saltlen] == ':' && memcmp(value, salt, saltlen) == 0) {
      result = memory.malloc(n);
      memcpy(result, value + saltlen + 1, n);
   } else {
      log_error(log, "Tampered salted value!!");
   }

   return result;
}

char *hashed(cad_memory_t memory, circus_log_t *log, const char *value) {
   assert(value != NULL);
   assert(value[0] != 0);

   gcry_md_hd_t hd;
   gcry_error_t e = gcrypt(md_open(&hd, GCRY_MD_SHA512, GCRY_MD_FLAG_SECURE));
   if (e != 0) {
      log_error(log, "Could not open hash algorithm");
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
      log_error(log, "Could not allocate memory for symmetric key");
   } else {
      log_debug(log, "Creating symmetric key (%d bits), needing entropy here...", KEY_SIZE * 8);
      gcry_randomize(raw, KEY_SIZE, GCRY_VERY_STRONG_RANDOM);
      log_debug(log, "Symmetric key created.");
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
   int len = strlen(value) + 1; // be sure to encrypt the '\0' at the end of the string, for correct decryption
   int n = len + KEY_SIZE - (len % KEY_SIZE);
   char *enc = memory.malloc(n);
   size_t key_size;
   key = unbase64(memory, b64key, &key_size);
   if (key == NULL) {
      log_error(log, "could not unbase64");
   } else {
      assert(key_size == KEY_SIZE);
      assert(gcry_cipher_get_algo_keylen(GCRY_CIPHER_AES256) == KEY_SIZE);
      e = gcrypt(cipher_setkey(hd, key, KEY_SIZE));
      if (e == 0) {
         size_t blklen = gcry_cipher_get_algo_blklen(GCRY_CIPHER_AES256);
         char *blk = memory.malloc(blklen);
         if (blk == NULL) {
            log_error(log, "could not allocate initialization vector");
         } else {
            memset(blk, 0, blklen);
            e = gcrypt(cipher_setiv(hd, blk, blklen));
            if (e == 0) {
               e = gcrypt(cipher_encrypt(hd, enc, n, value, len));
               if (e == 0) {
                  result = base64(memory, enc, n);
                  if (result == NULL) {
                     log_error(log, "could not base64");
                  } else {
                     log_pii(log, "len:%d|n:%d|result:%s", len, n, result);
                  }
               }
            }
            memory.free(blk);
         }
      }
      memory.free(key); // TODO try to free it earlier (to be tested)
   }
   memory.free(enc);
   gcry_cipher_close(hd);

   return result;
}

char *decrypted(cad_memory_t memory, circus_log_t *log, const char *b64value, const char *b64key) {
   assert(b64value != NULL);
   assert(b64value[0] != 0);

   size_t len;
   char *value = unbase64(memory, b64value, &len);
   if (value == NULL) {
      return NULL;
   }

   char *result = NULL;
   char *key;
   gcry_cipher_hd_t hd;
   gcry_error_t e = gcrypt(cipher_open(&hd, GCRY_CIPHER_AES256, GCRY_CIPHER_MODE_CFB, GCRY_CIPHER_SECURE));
   if (e != 0) {
      memory.free(value);
      return NULL;
   }
   size_t key_size;
   key = unbase64(memory, b64key, &key_size);
   if (key != NULL) {
      assert(key_size == KEY_SIZE);
      assert(gcry_cipher_get_algo_keylen(GCRY_CIPHER_AES256) == KEY_SIZE);
      e = gcrypt(cipher_setkey(hd, key, KEY_SIZE));
      if (e == 0) {
         size_t blklen = gcry_cipher_get_algo_blklen(GCRY_CIPHER_AES256);
         char *blk = memory.malloc(blklen);
         if (blk == NULL) {
            log_error(log, "could not allocate initialization vector");
         } else {
            memset(blk, 0, blklen);
            e = gcrypt(cipher_setiv(hd, blk, blklen));
            if (e == 0) {
               e = gcrypt(cipher_decrypt(hd, value, len, NULL, 0));
               if (e == 0) {
                  result = value;
               }
            }
            memory.free(blk);
         }
      }
      memory.free(key); // TODO try to free it earlier (to be tested)
   }
   gcry_cipher_close(hd);

   if (result != value) { // i.e. result == NULL
      memory.free(value);
   }

   return result;
}

unsigned int irandom(unsigned int max) {
   unsigned int result = 0;
   gcry_create_nonce((unsigned char*)&result, sizeof(unsigned int));
   result %= max;
   return result;
}

static char *szrandom_level(cad_memory_t memory, size_t len, enum gcry_random_level level, char *(*encode)(cad_memory_t, const char*, size_t)) {
   assert(len > 0);
   char *raw = memory.malloc(len + 1);
   if (raw == NULL) return NULL;
   gcry_randomize(raw, len, level);
   char *result = encode(memory, raw, len);
   memory.free(raw);
   return result;
}

char *szrandom32(cad_memory_t memory, size_t len) {
   return szrandom_level(memory, len, GCRY_STRONG_RANDOM, base32);
}

char *szrandom32_strong(cad_memory_t memory, size_t len) {
   return szrandom_level(memory, len, GCRY_VERY_STRONG_RANDOM, base32);
}

char *szrandom64(cad_memory_t memory, size_t len) {
   return szrandom_level(memory, len, GCRY_STRONG_RANDOM, base64);
}

char *szrandom64_strong(cad_memory_t memory, size_t len) {
   return szrandom_level(memory, len, GCRY_VERY_STRONG_RANDOM, base64);
}

int init_crypt(circus_log_t *log) {
   static int init = 0;
   gcry_error_t e;
   if (!init) {
      const char *ver = gcry_check_version(GCRYPT_VERSION);
      if (ver == NULL) {
         log_error(log, "gcrypt version mismatch");
         return 0;
      } else {
         log_info(log, "gcrypt version: %s", ver);
      }

      e = gcrypt(control(GCRYCTL_SUSPEND_SECMEM_WARN, 0));
      if (e != 0) {
         log_warning(log, "gcrypt suspend secmen warn failed");
      }
      e = gcrypt(control(GCRYCTL_INIT_SECMEM, 16384, 0));
      if (e != 0) {
         log_error(log, "gcrypt init secmen failed");
         return 0;
      }
      e = gcrypt(control(GCRYCTL_RESUME_SECMEM_WARN, 0));
      if (e != 0) {
         log_warning(log, "gcrypt resume secmen warn failed");
      }

      e = gcrypt(control(GCRYCTL_INITIALIZATION_FINISHED, 0));
      if (e != 0) {
         log_error(log, "gcrypt init failed");
         return 0;
      }
   }

   e = gcrypt(control(GCRYCTL_SELFTEST, 0));
   if (e != 0) {
      log_error(log, "gcrypt self-test failed");
      return 0;
   }

   return 1;
}

/* ---------------- TESTING FUNCTIONS ---------------- */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static char *WRAP_RANDOM = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
                           "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
                           "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
                           "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void __wrap_gcry_randomize(unsigned char *buffer, size_t length, enum gcry_random_level UNUSED(level)) {
   static char *random = NULL;
   static off_t size = 256;
   static off_t index = 0;
   if (random == NULL) {
      int fd = open(__FILE__, O_RDONLY | O_CLOEXEC);
      if (fd == -1) {
         random = WRAP_RANDOM;
      } else {
         random = malloc(size);
         ssize_t s = read(fd, random, size);
         if (s <= 0) {
            free(random);
            random = WRAP_RANDOM;
         } else {
            size = s;
         }
         close(fd);
      }
   }
   for (size_t i = 0; i < length; i++) {
      buffer[i] = random[index];
      index = (index + 1) % size;
   }
}

void __wrap_gcry_create_nonce(unsigned char *buffer, size_t length) {
   __wrap_gcry_randomize(buffer, length, 0);
}

void __wrap_gcry_randomize_rnd(unsigned char *buffer, size_t length, enum gcry_random_level UNUSED(level)) {
   static int fd = 0;
   if (fd == 0) {
      open("/dev/urandom", O_RDONLY | O_CLOEXEC);
   }
   if (fd != -1) {
      size_t n = 0;
      while (n < length) {
         ssize_t s = read(fd, buffer + n, length - n);
         if (s <= 0) {
            break;
         }
         n += (size_t)s;
      }
   }
}

void __wrap_gcry_create_nonce_rnd(unsigned char *buffer, size_t length) {
   __wrap_gcry_randomize_rnd(buffer, length, 0);
}
