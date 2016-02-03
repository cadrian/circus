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

#include "_test_server.h"

static int send_stop() {
   circus_message_query_stop_t *stop = new_circus_message_query_stop(stdlib_memory, "", "test");
   send_message(I(stop), NULL);
   I(stop)->free(I(stop));
   return 0;
}

int main(int argc, char **argv) {
   return test(argc, argv, send_stop);
}
