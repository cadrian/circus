#include <unistd.h>
#include <uv.h>

#include "log.h"

void wait_for_a_while(uv_idle_t* handle) {
   static int counter=0;

   log_info("main", "Idle %d", counter);

   counter++;
   if (counter > 10) {
      uv_idle_stop(handle);
      circus_log_end();
   }
}

int main(int argc, char **argv) {
   uv_idle_t idler;
   uv_idle_init(uv_default_loop(), &idler);
   uv_idle_start(&idler, wait_for_a_while);

   circus_set_format("%F:%L [%T] %O: %G\n");
   circus_start_log(NULL);

   uv_run(uv_default_loop(), UV_RUN_DEFAULT);
   uv_loop_close(uv_default_loop());

   return 0;
}
