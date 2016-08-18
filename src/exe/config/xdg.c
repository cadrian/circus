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

#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <circus_xdg.h>

static const char *xdg_data_home(void) {
   static const char *result = NULL;
   if (result == NULL) {
      char *xdg = getenv("XDG_DATA_HOME");
      int m = 0;
      if (xdg == NULL || xdg[0] == 0) {
         char *home = getenv("HOME");
         if (home != NULL) {
            xdg = szprintf(stdlib_memory, NULL, "%s/.local/share", home);
            assert(xdg != NULL);
            m = 1;
         }
      }
      if (xdg != NULL) {
         result = (const char *)szprintf(stdlib_memory, NULL, "%s/circus", xdg);
         if (m) {
            free(xdg);
         }
      }
   }
   return result;
}

static const char *xdg_config_home(void) {
   static const char *result = NULL;
   if (result == NULL) {
      char *xdg = getenv("XDG_CONFIG_HOME");
      int m = 0;
      if (xdg == NULL || xdg[0] == 0) {
         char *home = getenv("HOME");
         if (home != NULL) {
            xdg = szprintf(stdlib_memory, NULL, "%s/.config", home);
            m = 1;
         }
      }
      if (xdg != NULL) {
         result = (const char *)szprintf(stdlib_memory, NULL, "%s/circus", xdg);
         if (m) {
            free(xdg);
         }
      }
   }
   return result;
}

const char *xdg_data_dirs(void) {
   static const char *result = NULL;
   if (result == NULL) {
      char *xdg = getenv("XDG_DATA_DIRS");
      if (xdg == NULL || xdg[0] == 0) {
         xdg = "/var/run/circus/:/usr/share/circus/:/usr/local/share/circus/";
      }
      const char *xdh = xdg_data_home();
      if (xdh == NULL) {
         result = xdg;
      } else {
         result = szprintf(stdlib_memory, NULL, "%s:%s", xdh, xdg);
      }
   }
   return result;
}

const char *xdg_config_dirs(void) {
   static const char *result = NULL;
   if (result == NULL) {
      char *xdg = getenv("XDG_CONFIG_DIRS");
      if (xdg == NULL || xdg[0] == 0) {
         xdg = "/etc/xdg/circus:/usr/local/etc/circus";
      }
      const char *xch = xdg_config_home();
      if (xch == NULL) {
         result = xdg;
      } else {
         result = szprintf(stdlib_memory, NULL, "%s:%s", xch, xdg);
      }
   }
   return result;
}

const char *xdg_runtime_dir(void) {
   static const char *result = NULL;
   if (result == NULL) {
      char *xdg = getenv("XDG_RUNTIME_DIR");
      if (xdg == NULL || xdg[0] == 0) {
         char *tmp = getenv("TMPDIR");
         if (tmp != NULL) {
            xdg = szprintf(stdlib_memory, NULL, "%s/circus", tmp);
         } else {
            char *user = getenv("USER");
            if (user != NULL) {
               xdg = szprintf(stdlib_memory, NULL, "/tmp/circus-%s", user);
            } else {
               xdg = szprintf(stdlib_memory, NULL, "/tmp/circus-%d-runtime", getpid());
            }
         }
      }
      assert(xdg != NULL);
      result = (const char *)xdg;
   }
   return result;
}

read_t read_xdg_file_from_dirs(cad_memory_t memory, const char *filename, const char *dirs) {
   char *dirs0 = szprintf(memory, NULL, "%s", dirs);
   char *cur = dirs0, *next = cur;
   read_t result = { NULL, NULL, 0 };
   while (result.file == NULL && next != NULL) {
      memory.free(result.path);
      cur = next;
      next = strchr(cur, ':');
      if (next != NULL) {
         *next = '\0';
         next++;
      }
      result.path = szprintf(memory, NULL, "%s/%s", cur, filename);
      result.file = fopen(result.path, "r");
   }
   if (result.file == NULL) {
      memory.free(result.path);
      result.path = szprintf(memory, NULL, "%s/%s", dirs0, filename); // the first directory is returned as default
   }
   memory.free(dirs0);
   return result;
}
