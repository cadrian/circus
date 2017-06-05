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

#ifndef __CIRCUS_BASE64_H
#define __CIRCUS_BASE64_H

#include <cad_shared.h>

#include <circus.h>

/**
 * @ingroup circus_base64
 * @file
 *
 * An implementation of the base-64 encoding
 */

/**
 * @addtogroup circus_base64
 * @{
 */

/**
 * Convert a raw byte array size into its base64-encoded string length
 *
 * @param[in] len the raw byte array size
 * @return the base64 size needed to hold the base64-encoded byte array
 */
size_t b64_size(size_t len);

/**
 * Encode a byte array into a base64 string.
 *
 * @param[in] memory the memory allocator
 * @param[in] raw the byte array to encode
 * @param[in] len the size of the byte array
 * @return the string containing the byte array encoded in base64
 */
char *base64(cad_memory_t memory, const char *raw, size_t len);

/**
 * Decode a base64 string into a byte array. Even though not strictly
 * necessary the returned byte array is 0-terminated.
 *
 * @param[in] memory the memory allocator
 * @param[in] raw the byte array to encode
 * @param[out] len the size of the resulting byte array; may be NULL
 * @return the byte array, zero-terminated
 */
char *unbase64(cad_memory_t memory, const char *b64, size_t *len);

/**
 * @}
 */

#endif /* __CIRCUS_BASE64_H */
