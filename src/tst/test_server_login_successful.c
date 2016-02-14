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

static int send_login() {
   static const char *userid = "test";
   static const char *pass = "pass";
   int result = 0;

   circus_message_query_login_t *login = new_circus_message_query_login(stdlib_memory, "", pass, userid);
   circus_message_t *reply = NULL;
   send_message(I(login), &reply);
   if (reply == NULL) {
      printf("NULL login reply!\n");
      result = 1;
   } else {
      if (strcmp("login", reply->type(reply))) {
         printf("Invalid login reply: type is \"%s\"\n", reply->type(reply));
         result = 1;
      } else if (strcmp("reply", reply->command(reply))) {
         printf("Invalid login reply: command is \"%s\"\n", reply->command(reply));
         result = 1;
      } else {
         const char *error = reply->error(reply);
         if (strcmp(error, "")) {
            printf("Unexpected login error: %s\n", error);
            result = 2;
         } else {
            printf("Login OK.\n");
         }
      }
      reply->free(reply);
      I(login)->free(I(login));
   }

   circus_message_query_stop_t *stop = new_circus_message_query_stop(stdlib_memory, "", "test");
   send_message(I(stop), NULL);
   I(stop)->free(I(stop));

   return result;
}

int main(int argc, char **argv) {
   return test(argc, argv, send_login);
}
