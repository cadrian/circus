#include <circus_message_impl.h>

#include "_test_server.h"

static int send_stop() {
   circus_message_query_stop_t *stop = new_circus_message_query_stop(stdlib_memory, "", "test");
   send_message(I(stop), NULL);
   I(stop)->free(I(stop));
   return 0;
}

int main() {
   return test(send_stop);
}
