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
#ifndef __CIRCUS_CIRCUS_H
#define __CIRCUS_CIRCUS_H

#include <cad_shared.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define UNUSED(x) ((void)(x))

#ifdef DEBUG
#define crash() do { int *i ## __LINE__ = 0; *i ## __LINE__ = 0; } while(1)
#else
#define crash() do { fprintf(stderr, "Crash in %s:%d\n", __FILE__, __LINE__); exit(-1); } while(1)
#endif

#define assert(test)                                         \
   do {                                                      \
      if (!(test)) {                                         \
         fprintf(stderr, "**** ASSERT failed %s:%d: %s\n",   \
                 __FILE__, __LINE__, #test);                 \
         fflush(stderr);                                     \
         crash();                                            \
      }                                                      \
   }                                                         \
   while(0)

char *szprintf(cad_memory_t memory, int *size, const char *format, ...) __attribute__((format(printf, 3, 4)));
char *vszprintf(cad_memory_t memory, int *size, const char *format, va_list args);

#endif /* __CIRCUS_CIRCUS_H */