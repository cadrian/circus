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
*/

#ifndef __CIRCUS__TEST_H
#define __CIRCUS__TEST_H

#include <sqlite3.h>

void send_message(circus_message_t *query, circus_message_t **reply);
void *check_reply(circus_message_t *reply, const char *type, const char *command, const char *error);

void database(const char *query, int (*fn)(sqlite3_stmt*));

int do_login(const char *userid, const char *password, char **sessionid, char **token);

int test(int argc, char **argv, int (*fn)(void));

#endif /* __CIRCUS__TEST_H */
