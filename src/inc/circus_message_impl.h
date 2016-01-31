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

#ifndef __CIRCUS_MESSAGE_IMPL_H
#define __CIRCUS_MESSAGE_IMPL_H

#include <cad_array.h>
#include <circus_message.h>

#include "../exe/protocol/gen/factory.h"

union circus_message_visitor_s {
   circus_message_visitor_query_t query;
   circus_message_visitor_reply_t reply;
};

#endif /* __CIRCUS_MESSAGE_IMPL_H */
