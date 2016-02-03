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

#include <circus_message_impl.h>

#include "_test_server.h"

static int send_ping() {
   static const char *phrase = "Use the force, Luke.";
   int result = 0;

   circus_message_query_ping_t *ping = new_circus_message_query_ping(stdlib_memory, "", phrase);
   circus_message_t *reply = NULL;
   send_message(I(ping), &reply);
   if (reply == NULL) {
      printf("NULL ping reply!\n");
      result = 1;
   } else {
      if (strcmp("ping", reply->type(reply))) {
         printf("Invalid ping reply: type is \"%s\"\n", reply->type(reply));
         result = 1;
      } else if (strcmp("reply", reply->command(reply))) {
         printf("Invalid ping reply: command is \"%s\"\n", reply->command(reply));
         result = 1;
      } else {
         circus_message_reply_ping_t *pong = (circus_message_reply_ping_t*)reply;
         const char *p = pong->phrase(pong);
         if (strcmp(phrase, p)) {
            printf("Invalid ping reply: phrase is \"%s\"\n", p);
            result = 2;
         }
      }
      reply->free(reply);
      I(ping)->free(I(ping));
   }

   circus_message_query_stop_t *stop = new_circus_message_query_stop(stdlib_memory, "", "test");
   send_message(I(stop), NULL);
   I(stop)->free(I(stop));

   return result;
}

int main(int argc, char **argv) {
   return test(argc, argv, send_ping);
}
