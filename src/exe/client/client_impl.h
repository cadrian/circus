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

#ifndef __CIRCUS_CLIENT_IMPL_H
#define __CIRCUS_CLIENT_IMPL_H

#include <cad_cgi.h>
#include <cad_hash.h>

#include <circus_log.h>

#include "cgi_handler.h"

typedef struct {
   circus_client_cgi_handler_t fn;
   cad_memory_t memory;
   circus_log_t *log;
   circus_channel_t *channel;
   circus_automaton_t *automaton;
   char *templates_path;
} impl_cgi_t;

void set_response_string(impl_cgi_t *this, cad_cgi_response_t *response, int status, const char *string);
void set_response_template(impl_cgi_t *this, cad_cgi_response_t *response, int status, const char *template, cad_hash_t *extra);

#endif /* __CIRCUS_CLIENT_IMPL_H */
