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

#include <circus_message_impl.h>
#include <string.h>

#include "_test_server.h"

static int send_stop() {
   static const char *userid = "test";
   static const char *pass = "pass";
   circus_message_t *reply = NULL;
   int result = 0;

   char *sessionid = NULL;
   char *token = NULL;

   circus_message_query_stop_t *stop = new_circus_message_query_stop(stdlib_memory, "sessionid", "token", "test");
   send_message(I(stop), &reply);
   if (strcmp("refused", reply->error(reply)) != 0) {
      printf("Should have been refused\n");
   }

   result = do_login(userid, pass, &sessionid, &token);

   stop = new_circus_message_query_stop(stdlib_memory, sessionid, token, "test");
   reply->free(reply);
   send_message(I(stop), NULL);
   I(stop)->free(I(stop));

   stdlib_memory.free(sessionid);
   stdlib_memory.free(token);

   return result;
}

int main(int argc, char **argv) {
   return test(argc, argv, send_stop);
}
