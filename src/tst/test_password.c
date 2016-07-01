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
#include <uv.h>

#include <circus_password.h>
#include <circus_log.h>

circus_log_t *LOG;

int main() {
   LOG = circus_new_log_file_descriptor(stdlib_memory, LOG_PII, 1);
   char *pass;
   pass = generate_pass(stdlib_memory, LOG, "3a");
   assert(!strcmp(pass, "AsI"));
   free(pass);
   pass = generate_pass(stdlib_memory, LOG, "7ans");
   assert(!strcmp(pass, "AuG0?io"));
   free(pass);
   pass = generate_pass(stdlib_memory, LOG, "7an 2s");
   assert(!strcmp(pass, "u(QSs>wOy"));
   free(pass);
   pass = generate_pass(stdlib_memory, LOG, "14'azerty'");
   assert(!strcmp(pass, "eataatttaaeeee"));
   free(pass);
   LOG->free(LOG);
   return 0;
}
