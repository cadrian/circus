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

#include <string.h>

#include "circus.h"
#include "xdg.h"

const char *data_dirs(void) {
   static const char *result = NULL;
   if (!result) {
      char *xdg = getenv("XDG_DATA_DIRS");
      if (!xdg || !strlen(xdg)) {
         xdg = "/usr/local/share/:/usr/share/";
      }
      result = szprintf("%s:%s", data_home(), xdg);
   }
   return result;
}

const char *config_dirs(void) {
   static const char *result = NULL;
   if (!result) {
      char *xdg = getenv("XDG_CONFIG_DIRS");
      if (!xdg || !strlen(xdg)) {
         xdg = "/usr/local/etc:/etc/xdg";
      }
      result = szprintf("%s:%s", config_home(), xdg);
   }
   return result;
}

const char *cache_home(void) {
   static const char *result = NULL;
   if (!result) {
      char *xdg = getenv("XDG_CACHE_HOME");
      if (!xdg || !strlen(xdg)) {
         char *home = getenv("HOME");
         assert(home != NULL);
         xdg = szprintf("%s/.cache/circus", home);
         assert(xdg != NULL);
      }
      result = (const char *)xdg;
   }
   return result;
}

const char *runtime_dir(void) {
   static const char *result = NULL;
   if (!result) {
      char *xdg = getenv("XDG_RUNTIME_DIR");
      if (!xdg || !strlen(xdg)) {
         char *tmp = getenv("TMPDIR");
         if (tmp) {
            xdg = szprintf("%s/circus", tmp);
            assert(xdg != NULL);
         } else {
            char *user = getenv("USER");
            assert(user != NULL);
            xdg = szprintf("/tmp/circus-%s", user);
            assert(xdg != NULL);
         }
      }
      result = (const char *)xdg;
   }
   return result;
}

const char *data_home(void) {
   static const char *result = NULL;
   if (!result) {
      char *xdg = getenv("XDG_DATA_HOME");
      int m = 0;
      if (!xdg || !strlen(xdg)) {
         char *home = getenv("HOME");
         assert(home != NULL);
         xdg = szprintf("%s/.local/share", home);
         assert(xdg != NULL);
         m = 1;
      }
      result = (const char *)szprintf("%s/circus", xdg);
      if (m) {
         free(xdg);
      }
   }
   return result;
}

const char *config_home(void) {
   static const char *result = NULL;
   if (!result) {
      char *xdg = getenv("XDG_CONFIG_HOME");
      int m = 0;
      if (!xdg || !strlen(xdg)) {
         char *home = getenv("HOME");
         assert(home != NULL);
         xdg = szprintf("%s/.config", home);
         assert(xdg != NULL);
         m = 1;
      }
      result = (const char *)szprintf("%s/circus", xdg);
      if (m) {
         free(xdg);
      }
   }
   return result;
}
