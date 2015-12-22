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
#ifndef __CIRCUS_XDG_H
#define __CIRCUS_XDG_H

const char *data_dirs(void);
const char *config_dirs(void);

const char *cache_home(void);
const char *runtime_dir(void);
const char *data_home(void);
const char *config_home(void);

#endif /* __CIRCUS_XDG_H */
