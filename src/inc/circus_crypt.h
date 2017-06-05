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

#ifndef __CIRCUS_CRYPT_H
#define __CIRCUS_CRYPT_H

#include <circus.h>
#include <circus_log.h>

/**
 * @ingroup circus_crypt
 * @file
 *
 * Cryptography routines, based on libgcrypt.
 */

/**
 * @addtogroup circus_crypt
 * @{
 */

/**
 * Create a random salt value.
 *
 * @param[in] memory the memory allocator
 * @param[in] log the logger
 * @return the salt, in base64
 */
char *salt(cad_memory_t memory, circus_log_t *log);

/**
 * Salt a string.
 *
 * @param[in] memory the memory allocator
 * @param[in] log the logger
 * @param[in] salt the base64 salt returned by the @ref salt function
 * @param[in] value the string to salt
 * @return the salted string
 */
char *salted(cad_memory_t memory, circus_log_t *log, const char *salt, const char *value);

/**
 * Unsalt a string, i.e. perform the inverse function of @ref salted.
 *
 * @param[in] memory the memory allocator
 * @param[in] log the logger
 * @param[in] salt the base64 salt returned by the @ref salt function
 * (its value is checked)
 * @param[in] value the salted string
 * @return the unsalted string
 */
char *unsalted(cad_memory_t memory, circus_log_t *log, const char *salt, const char *value);

/**
 * Hash a string.
 *
 * @param[in] memory the memory allocator
 * @param[in] log the logger
 * @param[in] value the string to hash
 * @return the hash of the value, in base64
 */
char *hashed(cad_memory_t memory, circus_log_t *log, const char *value);

/**
 * Create a random symmetric key.
 *
 * @param[in] memory the memory allocator
 * @param[in] log the logger
 * @return the symmetric key, in base64
 */
char *new_symmetric_key(cad_memory_t memory, circus_log_t *log);

/**
 * Encrypt a string.
 *
 * @param[in] memory the memory allocator
 * @param[in] log the logger
 * @param[in] value the string to encrypt
 * @param[in] b64key the symmetric key returned by @ref new_symmetric_key
 * @return the bytes representing the encrypted string, in base64
 */
char *encrypted(cad_memory_t memory, circus_log_t *log, const char *value, const char *key);

/**
 * Decrypt a string.
 *
 * @param[in] memory the memory allocator
 * @param[in] log the logger
 * @param[in] b64value the bytes to decrypt, in base64
 * @param[in] b64key the symmetric key returned by @ref new_symmetric_key
 * @return the decrypted string
 */
char *decrypted(cad_memory_t memory, circus_log_t *log, const char *b64value, const char *key);

/**
 * NOTE!!! szrandom() and co produce longer buffers than len because
 * they are base32/64-encoded
 * @{
 */

/**
 * Generate a random sequence of bytes.
 *
 * @param[in] memory the memory allocator
 * @param[in] len the length of the bytes array
 * @return the bytes array, in human base32
 */
char *szrandom32(cad_memory_t memory, size_t len);

/**
 * Generate a "strong" random sequence of bytes.
 *
 * @param[in] memory the memory allocator
 * @param[in] len the length of the bytes array
 * @return the bytes array, in human base32
 */
char *szrandom32_strong(cad_memory_t memory, size_t len);

/**
 * Generate a random sequence of bytes.
 *
 * @param[in] memory the memory allocator
 * @param[in] len the length of the bytes array
 * @return the bytes array, in base64
 */
char *szrandom64(cad_memory_t memory, size_t len);

/**
 * Generate a "strong" random sequence of bytes.
 *
 * @param[in] memory the memory allocator
 * @param[in] len the length of the bytes array
 * @return the bytes array, in base64
 */
char *szrandom64_strong(cad_memory_t memory, size_t len);

/**
 * @}
 */

/**
 * Pick a random integer between 0 (included) and `max` (excluded).
 *
 * @param[in] max the maximum value of the random value to return, plus 1
 * @return a random integer
 */
unsigned int irandom(unsigned int max);

/**
 * Initialize the cryptography module
 *
 * @param[in] log the logger
 */
int init_crypt(circus_log_t *log);

/**
 * @}
 */

#endif /* __CIRCUS_CRYPT_H */
