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

// Based on http://fruli.krunch.be/~krunch/src/base32/base32.c
// adapted to the "human" flavour

#include <circus.h>
#include <string.h>

#include <cad_hash.h>

#include <circus_base32.h>

static char ALPHABET_L[] = "ybndrfg8ejkmcpqxot1uwisza345h769";
static char ALPHABET_H[] = "YBNDRFG8EJKMCPQXOT1UWISZA345H769";

static char encode_char(char c) {
   return ALPHABET_L[c & 0x1F];
}

static unsigned int b32_hash(const void *key) {
   return (unsigned int)*((char*)key);
}

static int b32_compare(const void *key1, const void *key2) {
   return b32_hash(key1) - b32_hash(key2);
}

static const void *b32_clone(const void *key) {
   return key;
}

static void b32_free(void *UNUSED(key)) {
   // nothing
}

static cad_hash_keys_t b32_keys = {b32_hash, b32_compare, b32_clone, b32_free};

static cad_hash_t *alphabet(void) {
   static cad_hash_t *map = NULL;
   if (map == NULL) {
      static int *values = NULL;
      size_t i, n = 32;
      assert(n == strlen(ALPHABET_H));
      assert(n == strlen(ALPHABET_L));
      values = malloc(n * sizeof(int));
      memset(values, 0, n * sizeof(int));
      map = cad_new_hash(stdlib_memory, b32_keys);
      for (i = 0; i < n; i++) {
         values[i] = i;
         map->set(map, &ALPHABET_L[i], &values[i]);
         if (ALPHABET_H[i] != ALPHABET_L[i]) {
            map->set(map, &ALPHABET_H[i], &values[i]);
         }
      }
   }
   return map;
}

static int decode_char(char c) {
   cad_hash_t *map = alphabet();
   void *res = map->get(map, &c);
   if (res == NULL) {
      return -1;
   }
   return *((int*)res);
}

static void pad(char *buf, int len) {
   for (int i = 0; i < len; i++) {
      buf[i] = '\0';
   }
}

#define min(a, b) ({ typeof (a) _a = (a); typeof (b) _b = (b); _a < _b ? _a : _b; })
#define get_octet(b) (((b) * 5) / 8)
#define get_offset(b) (8 - 5 - (5 * (b)) % 8)
#define shift_right(b, o) ((o) > 0 ? ((unsigned int)(b)) >> (o) : ((unsigned int)(b)) << -(o))
#define shift_left(b, o) shift_right(((unsigned int)(b)), -(o))

static void encode_sequence(const char *plain, int len, char *coded) {
   assert(len >= 0 && len <= 5);

   for (int block = 0; block < 8; block++) {
      int octet = get_octet(block);
      int junk = get_offset(block);

      if (octet >= len) {
         pad(&coded[block], 8 - block);
         return;
      }

      char c = shift_right(plain[octet], junk);

      if (junk < 0 && octet < len - 1) {
         c |= shift_right(plain[octet+1], 8 + junk);
      }
      coded[block] = encode_char(c);
   }
}

size_t b32_size(size_t len) {
   return ((len + 4) / 5 * 8);
}

char *base32(cad_memory_t memory, const char *raw, size_t len) {
   size_t sz = b32_size(len);
   char *coded = memory.malloc(sz + 1);
   memset(coded, 0, sz + 1);
   for (size_t i = 0, j = 0; i < len; i += 5, j += 8) {
      encode_sequence(&raw[i], min(len - i, (size_t)5), &coded[j]);
   }
   return coded;
}

static int decode_sequence(const char *coded, char *plain) {
   assert(coded && plain);

   plain[0] = '\0';
   for (int block = 0; block < 8; block++) {
      int offset = get_offset(block);
      int octet = get_octet(block);

      int c = decode_char(coded[block]);
      if (c < 0) {
         return octet;
      }

      plain[octet] |= shift_left(c, offset);
      if (offset < 0) {
         assert(octet < 4);
         plain[octet + 1] = shift_left(c, 8 + offset);
      }
   }
   return 5;
}

char *unbase32(cad_memory_t memory, const char *b32) {
   size_t sz = strlen(b32) * 5 / 8 + 8;
   int len = 0;
   char *raw = memory.malloc(sz + 1);
   memset(raw, 0, sz + 1);
   for (size_t i = 0, j = 0; ; i += 8, j += 5) {
      int n = decode_sequence(&b32[i], &raw[j]);
      len += n;
      if (n < 5) {
         break;
      }
   }
   assert((size_t)len <= sz);
   return raw;
}
