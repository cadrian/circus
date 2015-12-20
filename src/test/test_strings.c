#include "circus.h"

#include <string.h>

int main(int argc, char **argv) {
   char *message = szprintf("%s:%d", "test", 42);
   assert(!strcmp(message, "test:42"));
   return 0;
}
