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

    Copyright © 2015-2017 Cyril Adrian <cyril.adrian@gmail.com>
*/

#include <string.h>

#include <circus_message_impl.h>

#include "_test_server.h"

static int send_login() {
   static const char *userid = "test";
   static const char *pass = "pass";
   int result = 0;

   char *sessionid = NULL;
   char *token = NULL;

   result = do_login(userid, pass, &sessionid, &token);

   if (result == 0) {
      circus_message_t *reply = NULL;
      circus_message_query_logout_t *logout = new_circus_message_query_logout(stdlib_memory, sessionid, token);
      send_message(I(logout), &reply);
      circus_message_reply_logout_t *loggedout = check_reply(reply, "logout", "reply", "");
      I(logout)->free(I(logout));
      stdlib_memory.free(sessionid);
      stdlib_memory.free(token);
      if (loggedout == NULL) {
         printf("Logout should have succeeded\n");
         result = 1;
      } else {
         I(loggedout)->free(I(loggedout));
         // log in again to be able to stop the server :-)
         result = do_login(userid, pass, &sessionid, &token);
      }
   }

   circus_message_query_stop_t *stop = new_circus_message_query_stop(stdlib_memory, sessionid, token, "test");
   send_message(I(stop), NULL);
   I(stop)->free(I(stop));

   stdlib_memory.free(sessionid);
   stdlib_memory.free(token);

   return result;
}

int main(int argc, char **argv) {
   return test(argc, argv, send_login);
}
