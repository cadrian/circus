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

#ifndef __CIRCUS_CIRCUS_H
#define __CIRCUS_CIRCUS_H

#define _GNU_SOURCE

#include <cad_shared.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @ingroup circus_shared
 * @file
 *
 * Shared features, useful throughout the whole code.
 */

/**
 * @addtogroup circus_shared
 * @{
 */

/**
 * "Interface of". This macro allows to up-cast an implementation
 * object back to its interface.
 */
#define I(obj) (&(obj->fn))

/**
 * The UNUSED argument flag. Allows the code to document arguments
 * that are unused, either because they are defined in a callback that
 * does not care, or because the code has yet to be written...
 */
#ifdef UNUSED
#elif defined(__GNUC__)
#define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#else
#define UNUSED(x) x
#endif

/**
 * The crash function denotes a grave bug.
 */
#define crash() do { __crash(__FILE__, __LINE__); } while(1)
void __crash(const char *file, int line) __attribute__((noreturn));

/**
 * The assert macro denotes conditions that must always hold.
 */
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

/**
 * The classic "container of" macro. See the linux kernel :-)
 */
#define container_of(ptr, type, member) ({                      \
         const typeof( ((type *)0)->member ) *__mptr = (ptr);   \
         (type *)( (char *)__mptr - offsetof(type,member) );    \
      })

/**
 * Create a string using the given format and argument; similar to `snprintf(3)`.
 *
 * @param[in] memory the memory allocator
 * @param[out] size a pointer into which the size of the string can be stored; can be NULL
 * @param[in] format the format of the string
 * @param[in] ... the format arguments
 */
char *szprintf(cad_memory_t memory, int *size, const char *format, ...) __attribute__((format(printf, 3, 4)));

/**
 * Create a string using the given format and argument; similar to `vsnprintf(3)`.
 *
 * @param[in] memory the memory allocator
 * @param[out] size a pointer into which the size of the string can be stored; can be NULL
 * @param[in] format the format of the string
 * @param[in] args the format arguments
 */
char *vszprintf(cad_memory_t memory, int *size, const char *format, va_list args);

// ----------------------------------------------------------------
// Debugging guards

#if defined(__amd64__) || defined(__aarch64__)
#define CANARY (0x1235813215475129L)
#else
#define CANARY (0x12358132L)
#endif
#define SET_CANARY() volatile long int canary = CANARY
#define CHECK_CANARY() assert(canary == CANARY)

// ----------------------------------------------------------------
// Default configuration

/**
 * The default port the Circus server listens to.
 */
#define DEFAULT_PORT 4793

/**
 * @}
 */

#endif /* __CIRCUS_CIRCUS_H */
