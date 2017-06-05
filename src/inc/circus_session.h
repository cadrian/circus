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

#ifndef __CIRCUS_SESSION_H
#define __CIRCUS_SESSION_H

#include <circus.h>
#include <circus_log.h>
#include <circus_vault.h>

typedef struct circus_session_data_s circus_session_data_t;

typedef const char *(*circus_session_data_sessionid_fn)(circus_session_data_t *this);
typedef const char *(*circus_session_data_token_fn)(circus_session_data_t *this);
typedef const char *(*circus_session_data_set_token_fn)(circus_session_data_t *this);
typedef circus_user_t *(*circus_session_data_user_fn)(circus_session_data_t *this);

struct circus_session_data_s {
   circus_session_data_sessionid_fn sessionid;
   circus_session_data_token_fn token;
   circus_session_data_set_token_fn set_token;
   circus_session_data_user_fn user;
   // NO free(): managed by the session
};

typedef struct circus_session_s circus_session_t;

/*
 * WARNING: never keep references on data after having used them; they
 * may be freed by subsequent calls to the following functions.
 */

typedef circus_session_data_t *(*circus_session_get_fn)(circus_session_t *this, const char *sessionid, const char *token);
typedef circus_session_data_t *(*circus_session_set_fn)(circus_session_t *this, circus_user_t *user);
typedef void (*circus_session_free_fn)(circus_session_t *this);

struct circus_session_s {
   circus_session_get_fn get;
   circus_session_set_fn set;
   circus_session_free_fn free;
};

__PUBLIC__ circus_session_t *circus_session(cad_memory_t memory, circus_log_t *log, circus_config_t *config);

#endif /* __CIRCUS_SESSION_H */
