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

#ifndef __CIRCUS_CONFIG_H
#define __CIRCUS_CONFIG_H

#include <circus.h>

typedef struct circus_config_s circus_config_t;

typedef const char * (*circus_config_get_fn)(circus_config_t *this, const char *section, const char *key);
typedef void (*circus_config_set_fn)(circus_config_t *this, const char* section, const char *key, const char *value);
typedef void (*circus_config_write_fn)(circus_config_t *this);
typedef void (*circus_config_free_fn)(circus_config_t *this);

struct circus_config_s {
   circus_config_get_fn get;
   circus_config_set_fn set;
   circus_config_write_fn write;
   circus_config_free_fn free;
};

__PUBLIC__ circus_config_t *circus_config_read(cad_memory_t memory, const char *filename);

#endif /* __CIRCUS_CONFIG_H */
