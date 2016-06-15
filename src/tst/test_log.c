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

#include <unistd.h>
#include <uv.h>

#include <circus_log.h>

circus_log_t *LOG;

void wait_for_a_while(uv_idle_t* handle) {
   static int counter=0;

   log_info(LOG, "Idle %d", counter);

   counter++;
   if (counter > 10) {
      uv_idle_stop(handle);
      LOG->close(LOG);
   }
}

int main() {
   uv_idle_t idler;

   LOG = circus_new_log_file_descriptor(stdlib_memory, LOG_INFO, 1);

   uv_idle_init(uv_default_loop(), &idler);
   uv_idle_start(&idler, wait_for_a_while);

   LOG->set_format(LOG, "[%T] %O: %G\n");

   uv_run(uv_default_loop(), UV_RUN_DEFAULT);
   uv_loop_close(uv_default_loop());

   LOG->free(LOG);
   return 0;
}
