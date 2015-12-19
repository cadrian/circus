#include <uv.h>

#include "log/log.h"

void wait_for_a_while(uv_idle_t* handle) {
   static int counter=0;

   counter++;
   if (counter > 1) {
      uv_idle_stop(handle);
      circus_log_end();
   }
}

int main(void) {
   uv_idle_t idler;
   uv_idle_init(uv_default_loop(), &idler);
   uv_idle_start(&idler, wait_for_a_while);

   log_info("main", "%s", "This is a test");
   uv_run(uv_default_loop(), UV_RUN_DEFAULT);
   uv_loop_close(uv_default_loop());
}
