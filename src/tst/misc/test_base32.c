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

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <circus_base32.h>

int main() {
   char *test = "This is a base32 test.";
   printf("[%zd] %s\n", strlen(test), test);

   // Normal low-case encoding
   char *encoded = base32(stdlib_memory, test, strlen(test) + 1); // + 1 to encode the whole string with its terminator
   printf("[%zd] %s\n", strlen(encoded), encoded);
   size_t dl;
   char *decoded_l = unbase32(stdlib_memory, encoded, &dl);
   printf("[%zd] %s\n", dl, decoded_l);
   assert(dl == strlen(test) + 1);
   assert(!strcmp(decoded_l, test));

   // Check up-case encoding (base32 is case insensitive)
   char *p = encoded;
   while (*p) {
      *p = toupper(*p);
      p++;
   }
   printf("[%zd] %s\n", strlen(encoded), encoded);
   size_t dh;
   char *decoded_h = unbase32(stdlib_memory, encoded, &dh);
   printf("[%zd] %s\n", dh, decoded_h);
   assert(dh == strlen(test) + 1);
   assert(!strcmp(decoded_h, test));
}
