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

#include <circus.h>
#include <string.h>

#include <circus_base64.h>

// base64 implementation based on http://www.opensource.apple.com/source/QuickTimeStreamingServer/QuickTimeStreamingServer-452/CommonUtilitiesLib/base64.c

size_t b64_size(size_t len) {
   return ((len + 2) / 3 * 4);
}

char *base64(cad_memory_t memory, const char *raw, size_t len) {
   static char *ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
   static char PAD = '=';
   int n = b64_size(len) + 1;
   char *result = memory.malloc(n);
   if (result != NULL) {
      size_t i;
      char *p = result;
      for (i = 0; i < len - 2; i += 3) {
         *p++ = ALPHABET[(raw[i] >> 2) & 0x3F];
         *p++ = ALPHABET[((raw[i] & 0x3) << 4) | ((int) (raw[i + 1] & 0xF0) >> 4)];
         *p++ = ALPHABET[((raw[i + 1] & 0xF) << 2) | ((int) (raw[i + 2] & 0xC0) >> 6)];
         *p++ = ALPHABET[raw[i + 2] & 0x3F];
      }
      if (i < len) {
         *p++ = ALPHABET[(raw[i] >> 2) & 0x3F];
         if (i == (len - 1)) {
            *p++ = ALPHABET[((raw[i] & 0x3) << 4)];
            *p++ = PAD;
         }
         else {
            *p++ = ALPHABET[((raw[i] & 0x3) << 4) | ((int) (raw[i + 1] & 0xF0) >> 4)];
            *p++ = ALPHABET[((raw[i + 1] & 0xF) << 2)];
         }
         *p++ = PAD;
      }
      *p++ = '\0';
      assert(p == result + n);
   }
   return result;
}

char *unbase64(cad_memory_t memory, const char *b64, size_t *len) {
   assert(strlen(b64) % 4 == 0);

   static const char B64_TABLE[256] = {
      /*       0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f */
      /* 0 */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      /* 1 */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      /* 2 */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
      /* 3 */ 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 99, 64, 64,
      /* 4 */ 64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
      /* 5 */ 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
      /* 6 */ 64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
      /* 7 */ 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
      /* 8 */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      /* 9 */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      /* a */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      /* b */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      /* c */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      /* d */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      /* e */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      /* f */ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
   };

#define B64(i) (B64_TABLE[((int)b64[i]) & 0xff])
   char *end = strchrnul(b64, '=');
   size_t l = end - b64;
   size_t n = ((l + 3) / 4) * 3 + 1;
   char *result = memory.malloc(n);
   if (result != NULL) {
      char *p = result;
      while (l > 4) {
         *(p++) = (B64(0) << 2 | B64(1) >> 4);
         *(p++) = (B64(1) << 4 | B64(2) >> 2);
         *(p++) = (B64(2) << 6 | B64(3));
         l -= 4;
         b64 += 4;
      }
      assert(l != 1);
      if (l > 1) {
         *(p++) = (B64(0) << 2 | B64(1) >> 4);
      }
      if (l > 2) {
         *(p++) = (B64(1) << 4 | B64(2) >> 2);
      }
      if (l > 3) {
         *(p++) = (B64(2) << 6 | B64(3));
      }
      if (len != NULL) {
         *len = p - result;
      }
      assert(p < result + n);
      *(p++) = '\0';
   }
   return result;
#undef B64
}
