#include <string.h>

#include "circus.h"

int main(int argc, char **argv) {
   int len;
   char *message = szprintf(stdlib_memory, &len, "%s:%d", "test", 42);
   assert(!strcmp(message, "test:42"));
   assert(len == 7);
   return 0;
}
