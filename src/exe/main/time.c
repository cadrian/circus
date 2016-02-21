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

#include <circus_time.h>

struct timeval now(void) {
   struct timeval tv;
   int e = gettimeofday(&tv, NULL);
   assert(e == 0);
   return tv;
}

struct timeval __wrap_now(void) {
   static time_t start = (time_t)1456058518;
   struct timeval result = { start++, 0 };
   return result;
}
