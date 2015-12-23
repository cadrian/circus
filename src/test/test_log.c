#include <unistd.h>
#include <uv.h>

#include "log.h"

circus_log_t *LOG;

void wait_for_a_while(uv_idle_t* handle) {
   static int counter=0;

   cad_output_stream_t *info = LOG->stream(LOG, "main", LOG_INFO);
   info->put(info, "Idle %d", counter);

   counter++;
   if (counter > 10) {
      uv_idle_stop(handle);
      LOG->close(LOG);
   }
}

int main(int argc, char **argv) {
   uv_idle_t idler;

   LOG = circus_new_log_stdout(stdlib_memory, LOG_INFO);

   uv_idle_init(uv_default_loop(), &idler);
   uv_idle_start(&idler, wait_for_a_while);

   LOG->set_format(LOG, "[%T] %O: %G\n");

   uv_run(uv_default_loop(), UV_RUN_DEFAULT);
   uv_loop_close(uv_default_loop());

   LOG->free(LOG);
   return 0;
}
