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
#include <uv.h>

#include <circus_password.h>
#include <circus_log.h>

circus_log_t *LOG;
char *error;

static char *genpass(const char *recipe) {
   error = NULL;
   char *result = generate_pass(stdlib_memory, LOG, recipe, &error);
   if (error) {
      log_error(LOG, "recipe error: %s", error);
   }
   return result;
}

int main() {
   LOG = circus_new_log_file_descriptor(stdlib_memory, LOG_PII, 1);
   char *pass;
   pass = genpass("3a");
   assert(!error);
   assert(!strcmp(pass, "FVN"));
   free(pass);
   pass = genpass("7ans");
   assert(!error);
   assert(!strcmp(pass, "LpDpxc,"));
   free(pass);
   pass = genpass("s5-14an");
   assert(!error);
   assert(!strcmp(pass, "Bf9j<FD2w"));
   free(pass);
   pass = genpass("7an 2s");
   assert(!error);
   assert(!strcmp(pass, "2)<fBwFjD"));
   free(pass);
   pass = genpass("14'azerty'");
   assert(!error);
   assert(!strcmp(pass, "ryyeayaerzyzzr"));
   free(pass);

   pass = genpass("");
   assert(!pass);
   assert(!strcmp(error, "0: empty recipe"));

   pass = genpass("4");
   assert(!pass);
   assert(!strcmp(error, "2: Expecting ingredient specification"));

   pass = genpass("4q");
   assert(!pass);
   assert(!strcmp(error, "2: Invalid ingredient specification"));

   pass = genpass("4-q");
   assert(!pass);
   assert(!strcmp(error, "3: Invalid quantity range: min > max"));

   pass = genpass("\"");
   assert(!pass);
   assert(!strcmp(error, "2: Unterminated string"));

   pass = genpass("7'\\");
   assert(!pass);
   assert(!strcmp(error, "4: Unterminated string"));

   LOG->free(LOG);
   return 0;
}
