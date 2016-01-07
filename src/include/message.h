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

#ifndef __CIRCUS_MESSAGE_H
#define __CIRCUS_MESSAGE_H

#include <json.h>

typedef struct circus_message_s circus_message_t;
typedef struct circus_message_visitor_s circus_message_visitor_t;

typedef const char *(*circus_message_type_fn)(circus_message_t *this);
typedef const char *(*circus_message_command_fn)(circus_message_t *this);
typedef const char *(*circus_message_error_fn)(circus_message_t *this);
typedef void (*circus_message_accept_fn)(circus_message_t *this, circus_message_visitor_t *visitor);
typedef json_object_t *(*circus_message_serialize_fn)(circus_message_t *this);
typedef void (*circus_message_free_fn)(circus_message_t *this);

struct circus_message_s {
   circus_message_type_fn type;
   circus_message_command_fn command;
   circus_message_error_fn error;
   circus_message_accept_fn accept;
   circus_message_serialize_fn serialize;
   circus_message_free_fn free;
};

#endif /* __CIRCUS_MESSAGE_H */
