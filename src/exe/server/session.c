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

#include <cad_array.h>
#include <cad_hash.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

#include <circus_crypt.h>
#include <circus_session.h>

#define SESSIONID_LENGTH 128
#define TOKEN_LENGTH 128
#define TOKEN_RETENTION 5

typedef struct {
   circus_session_t fn;
   cad_memory_t memory;
   circus_log_t *log;
   cad_hash_t *per_user;
   cad_hash_t *per_sessionid;
   unsigned int sessionid_length;
   unsigned int token_length;
   unsigned int token_retention;
} session_impl_t;

typedef struct {
   circus_session_data_t fn;
   cad_memory_t memory;
   char *sessionid;
   cad_array_t *tokens;
   circus_user_t *user;
   session_impl_t *session;
} data_t;

static const char *rotate_tokens(data_t *this) {
   while (this->tokens->count(this->tokens) >= this->session->token_retention) {
      char *stale_token = *((char**)this->tokens->del(this->tokens, this->tokens->count(this->tokens)- 1));
      this->memory.free(stale_token);
   }
   char *new_token = szrandom(this->memory, this->session->token_length);
   this->tokens->insert(this->tokens, 0, &new_token);
   return new_token;
}

static const char *data_sessionid(data_t *this) {
   return this->sessionid;
}

static const char *data_token(data_t *this) {
   return *((char**)this->tokens->get(this->tokens, 0));
}

static const char *data_set_token(data_t *this) {
   return rotate_tokens(this);
}

static circus_user_t *data_user(data_t *this) {
   return this->user;
}

static circus_session_data_t data_fn = {
   (circus_session_data_sessionid_fn)data_sessionid,
   (circus_session_data_token_fn)data_token,
   (circus_session_data_set_token_fn)data_set_token,
   (circus_session_data_user_fn)data_user,
};

static data_t *new_data(cad_memory_t memory, circus_user_t *user, session_impl_t *session) {
   data_t *result = memory.malloc(sizeof(data_t));
   assert(result != NULL);
   result->fn = data_fn;
   result->memory = memory;
   result->sessionid = szrandom(memory, session->sessionid_length);
   result->tokens = cad_new_array(memory, sizeof(char*));
   result->user = user;
   result->session = session;

   rotate_tokens(result);
   assert(result->sessionid != NULL);
   assert(result->tokens->count(result->tokens) > 0);

   return result;
}

static void free_data(data_t *data) {
   while (data->tokens->count(data->tokens) > 0) {
      char *del_token = *((char**)data->tokens->del(data->tokens, 0));
      data->memory.free(del_token);
   }
   data->memory.free(data->sessionid);
   data->tokens->free(data->tokens);
   data->memory.free(data);
}

// ----------------------------------------------------------------

static unsigned int user_hash(circus_user_t *user) {
   return cad_hash_strings.hash(user->name(user));
}

static int user_compare(circus_user_t *user1, circus_user_t *user2) {
   return cad_hash_strings.compare(user1->name(user1), user2->name(user2));
}

static circus_user_t *user_clone(circus_user_t *user) {
   return user;
}

static void user_free(circus_user_t *UNUSED(user)) {
   // do nothing
}

static cad_hash_keys_t hash_user_keys = {
   (cad_hash_keys_hash_fn)user_hash,
   (cad_hash_keys_compare_fn)user_compare,
   (cad_hash_keys_clone_fn)user_clone,
   (cad_hash_keys_free_fn)user_free,
};

// ----------------------------------------------------------------

static circus_session_data_t *session_get(session_impl_t *this, const char *sessionid, const char *token) {
   unsigned int i;
   data_t *data = this->per_sessionid->get(this->per_sessionid, sessionid);
   if (data != NULL) {
      assert(strncmp(data->sessionid, sessionid, this->sessionid_length) == 0);
      for (i = 0; i < data->tokens->count(data->tokens); i++) {
         const char *check_token = *((char**)data->tokens->get(data->tokens, i));
         if (!strncmp(token, check_token, this->token_length)) {
            return I(data);
         }
      }
   }
   return NULL;
}

static circus_session_data_t *session_set(session_impl_t *this, circus_user_t *user) {
   data_t *data = this->per_user->get(this->per_user, user);
   if (data != NULL) {
      assert(data->user == user);
      this->per_sessionid->del(this->per_sessionid, data->sessionid);
      this->per_user->del(this->per_user, user);
      free_data(data);
   }
   data = new_data(this->memory, user, this);
   this->per_sessionid->set(this->per_sessionid, data->sessionid, data);
   this->per_user->set(this->per_user, user, data);
   return I(data);
}

static void clean_sessionid(void *UNUSED(hash), int UNUSED(index), const char *UNUSED(key), data_t *UNUSED(value), session_impl_t *UNUSED(this)) {
   // Avoid double free() by not doing it :-)
}

static void clean_user(void *UNUSED(hash), int UNUSED(index), const circus_user_t *UNUSED(key), data_t *value, session_impl_t *UNUSED(this)) {
   free_data(value);
}

static void session_free(session_impl_t *this) {
   this->per_sessionid->clean(this->per_sessionid, (cad_hash_iterator_fn)clean_sessionid, this);
   this->per_user->clean(this->per_user, (cad_hash_iterator_fn)clean_user, this);
   this->per_sessionid->free(this->per_sessionid);
   this->per_user->free(this->per_user);
   this->memory.free(this);
}

static circus_session_t session_fn = {
   (circus_session_get_fn)session_get,
   (circus_session_set_fn)session_set,
   (circus_session_free_fn)session_free,
};

circus_session_t *circus_session(cad_memory_t memory, circus_log_t *log, circus_config_t *config) {
   session_impl_t *result = memory.malloc(sizeof(session_impl_t));
   assert(result != NULL);

   result->fn = session_fn;
   result->memory = memory;
   result->log = log;
   result->per_user = cad_new_hash(memory, hash_user_keys);
   result->per_sessionid = cad_new_hash(memory, cad_hash_strings);

   result->sessionid_length = SESSIONID_LENGTH;
   result->token_length = TOKEN_LENGTH;
   result->token_retention = TOKEN_RETENTION;

   const char *sz_sessionid_length = config->get(config, "session", "sessionid_length");
   if (sz_sessionid_length != NULL) {
      errno = 0;
      unsigned long int t = strtoul(sz_sessionid_length, NULL, 10);
      if ((t != ULONG_MAX || errno != ERANGE) && errno != EINVAL && t > 0 && t < UINT_MAX) {
         result->sessionid_length = (int)t;
      } else {
         log_error(log, "Invalid sessionid_length: %s", sz_sessionid_length);
      }
   }

   const char *sz_token_length = config->get(config, "session", "token_length");
   if (sz_token_length != NULL) {
      errno = 0;
      unsigned long int t = strtoul(sz_token_length, NULL, 10);
      if ((t != ULONG_MAX || errno != ERANGE) && errno != EINVAL && t > 0 && t < UINT_MAX) {
         result->token_length = (int)t;
      } else {
         log_error(log, "Invalid token_length: %s", sz_token_length);
      }
   }

   const char *sz_token_retention = config->get(config, "session", "token_retention");
   if (sz_token_retention != NULL) {
      errno = 0;
      unsigned long int t = strtoul(sz_token_retention, NULL, 10);
      if ((t != ULONG_MAX || errno != ERANGE) && errno != EINVAL && t > 0 && t < UINT_MAX) {
         result->token_retention = (int)t;
      } else {
         log_error(log, "Invalid token_retention: %s", sz_token_retention);
      }
   }

   assert(result->per_user != NULL);
   assert(result->per_sessionid != NULL);

   return I(result);
}
