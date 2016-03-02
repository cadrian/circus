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

   circus_message_query_ping_t *ping = new_circus_message_query_ping(stdlib_memory, phrase);
   circus_message_t *reply = NULL;
   send_message(I(ping), &reply);
   circus_message_reply_ping_t *pong = check_reply(reply, "ping", "reply", "");
   if (pong == NULL) {
      result = 1;
   } else {
      const char *p = pong->phrase(pong);
      if (strcmp(phrase, p)) {
         printf("Invalid ping reply: phrase is \"%s\"\n", p);
         result = 2;
      }
      reply->free(reply);
   }
   I(ping)->free(I(ping));

   circus_message_query_login_t *login = new_circus_message_query_login(stdlib_memory, "test", "pass");
   send_message(I(login), &reply);
   circus_message_reply_login_t *loggedin = check_reply(reply, "login", "reply", "");
   I(login)->free(I(login));

   circus_message_query_stop_t *stop = new_circus_message_query_stop(stdlib_memory, loggedin->sessionid(loggedin), loggedin->token(loggedin), "test");
   reply->free(reply);
   send_message(I(stop), NULL);
   I(stop)->free(I(stop));

   return result;
}

int main(int argc, char **argv) {
   return test(argc, argv, send_ping);
}
