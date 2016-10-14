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

#ifndef __CIRCUS_BASE32_H
#define __CIRCUS_BASE32_H

#include <cad_shared.h>

#include <circus.h>

/**
 * @ingroup circus_base32
 * @file
 *
 * An implementation of the "human-oriented base-32" encoding
 */

/**
 * @addtogroup circus_base32
 * @{
 */

/**
 * Convert a raw byte array size into its base32-encoded string length
 *
 * @param[in] len the raw byte array size
 * @return the base32 size needed to hold the base32-encoded byte array
 */
size_t b32_size(size_t len);

/**
 * Encode a byte array into a base32 string.
 *
 * @param[in] memory the memory allocator
 * @param[in] raw the byte array to encode
 * @param[in] len the size of the byte array
 * @return the string containing the byte array encoded in base32
 */
char *base32(cad_memory_t memory, const char *raw, size_t len);

/**
 * Decode a base32 string into a byte array.
 *
 * @param[in] memory the memory allocator
 * @param[in] raw the byte array to encode
 * @param[out] len the size of the resulting byte array; may be NULL
 * @return the byte array
 */
char *unbase32(cad_memory_t memory, const char *b32, size_t *len);

/**
 * @}
 */

#endif /* __CIRCUS_BASE32_H */
