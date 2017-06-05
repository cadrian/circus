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

#ifndef __CIRCUS_XDG_H
#define __CIRCUS_XDG_H

#include <circus.h>

const char *xdg_data_dirs(void);
const char *xdg_config_dirs(void);
const char *xdg_runtime_dir(void);

typedef struct {
   FILE *file;
   char *path;
   int local;
} read_t;

read_t read_xdg_file_from_dirs(cad_memory_t memory, const char *filename, const char *dirs);

#endif /* __CIRCUS_XDG_H */
