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

#include <stdio.h>
#include <string.h>

#include "circus.h"

char *vszprintf(const char *format, va_list args) {
   va_list args2;
   int n, n2;
   char *result = NULL;

   va_copy(args2, args);
   n = vsnprintf("", 0, format, args);
   assert(n >= 0);
   result = malloc(n + 1);
   assert(result != NULL);
   n2 = vsnprintf(result, n + 1, format, args2);
   assert(n2 == n);
   va_end(args2);

   return result;
}

char *szprintf(const char *format, ...) {
   va_list args;
   char *result = NULL;
   va_start(args, format);
   result = vszprintf(format, args);
   va_end(args);
   assert(result != NULL);
   return result;
}

#include "circus.h"
