#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "circus.h"
#include "xdg.h"

int main(int argc, char **argv) {
   setenv("XDG_DATA_DIRS", "/xdg/data/dirs", 1);
   setenv("XDG_CONFIG_DIRS", "/xdg/config/dirs", 1);
   setenv("XDG_CACHE_HOME", "/xdg/cache/home", 1);
   setenv("XDG_RUNTIME_DIR", "/xdg/runtime/dir", 1);
   setenv("XDG_DATA_HOME", "/xdg/data/home", 1);
   setenv("XDG_CONFIG_HOME", "/xdg/config/home", 1);

   setenv("HOME", "/home/test", 1);
   setenv("USER", "test", 1);

   printf("data_dirs = %s\n", xdg_data_dirs());
   printf("config_dirs = %s\n", xdg_config_dirs());
   printf("cache_home = %s\n", xdg_cache_home());
   printf("runtime_dir = %s\n", xdg_runtime_dir());
   printf("data_home = %s\n", xdg_data_home());
   printf("config_home = %s\n", xdg_config_home());

   return 0;
}
