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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <circus_xdg.h>

int main() {
   unsetenv("XDG_DATA_DIRS");
   unsetenv("XDG_CONFIG_DIRS");
   unsetenv("XDG_CACHE_HOME");
   unsetenv("XDG_RUNTIME_DIR");
   unsetenv("TMPDIR");
   unsetenv("XDG_DATA_HOME");
   unsetenv("XDG_CONFIG_HOME");

   setenv("HOME", "/home/test", 1);
   setenv("USER", "test", 1);

   printf("data_dirs = %s\n", xdg_data_dirs());
   printf("config_dirs = %s\n", xdg_config_dirs());
   printf("runtime_dir = %s\n", xdg_runtime_dir());

   return 0;
}
