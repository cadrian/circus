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

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

#include <circus.h>
#include <circus_memory.h>

#pragma GCC push_options
#pragma GCC optimize ( "-O0" )

static volatile size_t __bzero_max = 1024;

/*
 * Try to be really sure that the compiler does not optimize this:
 *  - force -O0
 *  - reverse loop to force cache misses
 *  - at least __bzero_max loops; so this should be safe if count < __bzero_max
 */
__attribute__ (( noinline )) void force_bzero(void *buf, size_t count) {
   volatile char* data = (volatile char*)buf;
   volatile size_t i;
   volatile size_t max = count > __bzero_max ? count : __bzero_max;
   for (i = max; i --> 0; ) {
      data[i % count] = '\0';
   }
}

__attribute__ (( noinline )) size_t max_bzero(size_t count) {
   volatile size_t result = __bzero_max;
   volatile size_t r;
   volatile size_t i;
   for (i = 0; i < 16; i++) {
      if (result < count) {
         r = result * 2;
         if (r > result) {
            result = r; // beware of overflows
         }
      }
   }
   __bzero_max = result;
   return result;
}

typedef struct {
   size_t size;
   volatile long canary;
   char data[0];
} mem;

static void circus_memfree(mem *p) {
   assert(p->canary == CANARY);
   size_t size = p->size;
   assert(size > 0);
   max_bzero(size);
   force_bzero(p, sizeof(mem) + size);
   munlock(p, sizeof(mem) + size);
   free(p);
}

static mem *circus_memalloc(size_t size) {
   size_t s = sizeof(mem) + size;
   mem *result = malloc(s);
   if (result != NULL) {
      result->size = (uintptr_t)size;
      result->canary = CANARY;
      int n = mlock(result, s);
      if (n != 0) {
         fprintf(stderr, "mlock error: %d - %s\n", errno, strerror(errno));
         circus_memfree(result);
         result = NULL;
      } else if (size > 0) {
         memset(&(result->data), 0, size);
      }
   }
   return result;
}

static void *circus_malloc(size_t size) {
   mem *result = circus_memalloc(size);
   if (result == NULL) {
      return NULL;
   }
   return result->data;
}

static void *circus_realloc(void *ptr, size_t size) {
   if (ptr == NULL) {
      return circus_malloc(size);
   }
   mem *p = container_of(ptr, mem, data);
   assert(p->canary == CANARY);
   if (size <= p->size) {
      return ptr;
   }
   mem *result = circus_memalloc(size);
   if (result == NULL) {
      circus_memfree(p);
      return NULL;
   }
   memcpy(result->data, p->data, p->size);
   circus_memfree(p);
   return result->data;
}

static void circus_free(void *ptr) {
   if (ptr != NULL) {
      circus_memfree(container_of(ptr, mem, data));
   }
}

cad_memory_t MEMORY = {circus_malloc, circus_realloc, circus_free};

#pragma GCC pop_options

int __wrap_mlock(const void *UNUSED(addr), size_t UNUSED(len)) {
   return 0;
}

int __wrap_munlock(const void *UNUSED(addr), size_t UNUSED(len)) {
   return 0;
}
