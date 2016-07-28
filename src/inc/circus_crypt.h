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

#ifndef __CIRCUS_CRYPT_H
#define __CIRCUS_CRYPT_H

#include <circus.h>
#include <circus_log.h>

/* Encryption routines */

char *salt(cad_memory_t memory, circus_log_t *log);
char *salted(cad_memory_t memory, circus_log_t *log, const char *salt, const char *value);
char *unsalted(cad_memory_t memory, circus_log_t *log, const char *salt, const char *value);
char *hashed(cad_memory_t memory, circus_log_t *log, const char *value);
char *new_symmetric_key(cad_memory_t memory, circus_log_t *log);
char *encrypted(cad_memory_t memory, circus_log_t *log, const char *value, const char *key);
char *decrypted(cad_memory_t memory, circus_log_t *log, const char *b64value, const char *key);

/* NOTE!!! szrandom() and co produce longer buffers than len because they are base32/64-encoded */

char *szrandom32(cad_memory_t memory, size_t len);
char *szrandom32_strong(cad_memory_t memory, size_t len);
char *szrandom64(cad_memory_t memory, size_t len);
char *szrandom64_strong(cad_memory_t memory, size_t len);
unsigned int irandom(unsigned int max);

/* init */

int init_crypt(circus_log_t *log);

#endif /* __CIRCUS_CRYPT_H */
