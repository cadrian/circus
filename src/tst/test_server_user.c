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
   int result = 0;

   char *admin_sessionid = NULL;
   char *admin_token = NULL;

   result = do_login("test", "pass", &admin_sessionid, &admin_token);
   if (result == 0) {
      circus_message_t *reply = NULL;
      circus_message_query_user_t *admin_user = new_circus_message_query_user(stdlib_memory, "", admin_sessionid, admin_token, "noob", "noob@clueless.lol", "user");
      send_message(I(admin_user), &reply);
      circus_message_reply_user_t *admin_userr = check_reply(reply, "user", "reply", "");
      if (admin_userr == NULL) {
         result = 1;
      } else {
         stdlib_memory.free(admin_token);
         admin_token = szprintf(stdlib_memory, NULL, "%s", admin_userr->token(admin_userr));
         const char *p = admin_userr->password(admin_userr);
         const char *v = admin_userr->validity(admin_userr);
         if (p == NULL || *p == 0) {
            printf("Unexpected null or empty generated password");
            result = 1;
         } else if (v == NULL || *v == 0) {
            printf("Unexpected null or empty validity");
            result = 1;
         } else {
            char *password = szprintf(stdlib_memory, NULL, "%s", p);
            stdlib_memory.free(reply);

            char *user_sessionid = NULL;
            char *user_token = NULL;

            result = do_login("noob", password, &user_sessionid, &user_token);
            if (result != 0) {
               printf("Login should have succeeded\n");
            }

            stdlib_memory.free(password);
            stdlib_memory.free(user_sessionid);
            stdlib_memory.free(user_token);
         }
      }
   }

   circus_message_query_stop_t *stop = new_circus_message_query_stop(stdlib_memory, "", admin_sessionid, admin_token, "test");
   send_message(I(stop), NULL);
   I(stop)->free(I(stop));

   stdlib_memory.free(admin_sessionid);
   stdlib_memory.free(admin_token);

   return result;
}

int main(int argc, char **argv) {
   return test(argc, argv, send_login);
}
