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

#include <stdio.h>
#include <string.h>

#include <circus_base64.h>

int main() {
   char *test = "This is a base64 test.";
   printf("%s\n", test);
   char *encoded = base64(stdlib_memory, test, strlen(test) + 1); // + 1 to encode the whole string with its terminator
   printf("%s\n", encoded);
   char *decoded = unbase64(stdlib_memory, encoded);
   printf("%s\n", decoded);
   assert(!strcmp(decoded, test));
}
