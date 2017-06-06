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

#ifndef __CIRCUS_VAULT_HASH_H
#define __CIRCUS_VAULT_HASH_H

#include <circus_log.h>
#include <inttypes.h>

/*
 * Password hashing algorithms
 *
 * See https://nakedsecurity.sophos.com/2013/11/20/serious-security-how-to-store-your-users-passwords-safely/
 */

#define DEFAULT_STRETCH ((uint64_t)65536)

typedef struct {
   uint64_t stretch;
   char *clear;
   char *salt;
   char *hashed;
} hashing_t;

int pass_hash(cad_memory_t memory, circus_log_t *log, hashing_t *hashing);
int pass_compare(cad_memory_t memory, circus_log_t *log, hashing_t *hashing, uint64_t min_stretch);

#endif /* __CIRCUS_VAULT_HASH_H */
