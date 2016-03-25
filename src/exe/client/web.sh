#!/usr/bin/env bash

# This file is part of Circus.
#
# Circus is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License.
#
# Circus is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Circus.  If not, see <http://www.gnu.org/licenses/>.
#
# Copyright © 2015-2016 Cyril Adrian <cyril.adrian@gmail.com>

set -e
set -u

# Yet another ugly script that generates code.
# This script generates the web framework using the web.json file.

cd $(dirname $(readlink -f "$0"))

rm -rf gen
mkdir gen

JQ=$(which jq) || {
    echo "jq not installed!!" >&2
    exit 1
}
function jq() {
    for item in $(eval echo $($JQ "$@ | @sh" < web.json)); do
        eval echo $item
    done
}

DATE="$(date -R)"
function init_file() {
    local name=$(pwd)/gen/"$1"
    test -f $name || {
        mkdir -p $(dirname $name)
        echo "// Generated on $DATE - don't edit!" > $name
    }
    echo $name
}

function for_all() {
    for fn in "$@"; do
        do_$fn
    done
    jq 'keys' | while read url; do
        echo URL: "$url"
        for fn in "$@"; do
            url_$fn "$url"
        done
        jq '.["'$url'"] | keys' | while read action; do
            echo ACTION: "$action"

            local query=$(jq '.["'$url'"]["'$action'"]["query"]')
            local reply=$(jq '.["'$url'"]["'$action'"]["reply"]')
            local params=$(jq '.["'$url'"]["'$action'"]["params"]')
            local extra=$(jq '.["'$url'"]["'$action'"]["extra"]')
            local cookie_read=$(jq '.["'$url'"]["'$action'"]["cookie"]["read"]')
            local cookie_write=$(jq '.["'$url'"]["'$action'"]["cookie"]["write"]')
            local template=$(jq '.["'$url'"]["'$action'"]["template"]')

            echo "query: $query"
            echo "reply: $reply"
            echo "params: $params"
            echo "extra: $extra"
            echo "cookie: read ${cookie_read} - write ${cookie_write}"
            echo "template: $template"

            query_$fn "$query" "$params" "$cookie_read"
            reply_$fn "$reply" "$extra" "$cookie_read" "$cookie_write" "$template"
        done
        for fn in "$@"; do
            lru_$fn "$url"
        done
    done
    for fn in "$@"; do
        od_$fn
    done
}

# ----------------------------------------------------------------
# WEB HEADER

function do_webh() {
    local file=$(init_file web.h)
    {
        echo '#ifndef __CIRCUS_WEB_H'
        echo '#define __CIRCUS_WEB_H'
        echo '#include "../client_impl.h"'
        echo 'void post_read(impl_cgi_t *this, cad_cgi_response_t *response);'
        echo 'void post_write(impl_cgi_t *this, cad_cgi_response_t *response);'
    } >> $file
}

function od_webh() {
    local file=$(init_file web.h)
    {
        echo '#endif /* __CIRCUS_WEB_H */'
    } >> $file
}

function url_webh() {
    local url=$1
}

function lru_webh() {
    local url=$1
}

function query_webh() {
    local query="$1"
    local params=("${2[@]}")
    local cookie_read="$3"
}

function reply_webh() {
    local reply="$1"
    local extra=("${2[@]}")
    local cookie_read="$3"
    local cookie_write="$4"
    local template="$5"
}

# ----------------------------------------------------------------
# WEB CODE

function do_webc() {
    local file=$(init_file web.c)
    {
        echo '#include <string.h>'
        echo
        echo '#include <circus_message_impl.h>'
        echo '#include "web.h"'
        echo
        echo 'void post_read(impl_cgi_t *this, cad_cgi_response_t *response) {'
        echo '   circus_message_t *message = NULL;'
        echo '   cad_cgi_meta_t *meta = response->meta_variables(response);'
        echo '   const char *path = meta->path_info(meta);'
        echo '   cad_hash_t *form = meta->input_as_form(meta);'
        echo '   assert(form != NULL);'
        echo '#include "post_read.c"'
        echo '   if (message == NULL) {'
        echo '      set_response_string(this, response, 401, "Invalid path");'
        echo '   } else {'
        echo '      this->automaton->set_state(this->automaton, State_write_to_server, message);'
        echo '   }'
        echo '}'
        echo
        echo 'void post_write(impl_cgi_t *this, cad_cgi_response_t *response) {'
        echo '   circus_message_t *message = this->automaton->message(this->automaton);'
        echo '   const char *error = message->error(message);'
        echo '   if (strlen(error) == 0) {'
        echo '      cad_cgi_meta_t *meta = response->meta_variables(response);'
        echo '      const char *path = meta->path_info(meta);'
        echo '#include "post_write.c"'
        echo '   } else {'
        echo '      log_error(this->log, "web", "error: %s", error);'
        echo '      set_response_string(this, response, 403, "Access denied.");'
        echo '   }'
        echo '}'
    } >> $file
}

function od_webc() {
    local read_file=$(init_file post_read.c)
    {
        echo '{'
        echo '   set_response_string(this, response, 404, "Not found.");'
        echo '}'
    } >> $read_file
    local write_file=$(init_file post_write.c)
    {
        echo '{'
        echo '   set_response_string(this, response, 404, "Not found.");'
        echo '}'
    } >> $write_file
}

function url_webc() {
    local url=$1
    local read_file=$(init_file post_read.c)
    {
        echo 'if (!strcmp(path, "'"$url"'")) {'
    } >> $read_file
    local write_file=$(init_file post_write.c)
    {
        echo 'if (!strcmp(path, "'"$url"'")) {'
    } >> $write_file
}

function lru_webc() {
    local url=$1
    local read_file=$(init_file post_read.c)
    {
        echo -n '} else '
    } >> $read_file
    local write_file=$(init_file post_write.c)
    {
        echo -n '} else '
    } >> $write_file
}

function query_webc() {
    local query="$1"
    local params="$2"
    local cookie_read="$3"
    local read_file=$(init_file post_read.c)
    {
        for param in $params; do
            echo "   const char *${query}_$param = form->get(form, \"$param\");"
        done
        if [ "$cookie_read" != null ]; then
            echo "   cad_cgi_cookies_t *${query}_cookies = response->cookies(response);"
            echo "   cad_cgi_cookie_t *${query}_${cookie_read}_websid = ${query}_cookies->get(${query}_cookies, \"WEBSID\");"
            echo "   const char *${query}_${cookie_read} = ${query}_${cookie_read}_websid->value(${query}_${cookie_read}_websid);"
        fi
        echo -n "   message = I(new_circus_message_query_$query(this->memory"
        if [ "$cookie_read" != null ]; then
            echo -n ", ${query}_${cookie_read}"
        fi
        for param in $params; do
            echo -n ", ${query}_$param"
        done
        echo '));'
    } >> $read_file
}

function reply_webc() {
    local reply="$1"
    local extra="$2"
    local cookie_read="$3"
    local cookie_write="$4"
    local template="$5"
    local write_file=$(init_file post_write.c)
    {
        echo "   if (!strcmp(message->type(message), \"$reply\") && !strcmp(message->command(message), \"reply\")) {"
        echo "      circus_message_reply_${reply}_t *${query}_reply = (circus_message_reply_${reply}_t*)message;"
        echo "      cad_hash_t *${query}_extra = cad_new_hash(this->memory, cad_hash_strings);"
        for extrum in $extra; do
            echo "      ${query}_extra->set(${query}_extra, \"$extrum\", (char*)${query}_reply->$extrum(${query}_reply));"
        done
        echo "      set_response_template(this, response, 200, \"$template\", ${query}_extra);"
        echo "      ${query}_extra->free(${query}_extra);"
        if [ "$cookie_write" != null ]; then
            echo "      log_debug(this->log, \"web\", \"Setting cookie: ${cookie_write}\");"
            echo "      cad_cgi_cookies_t *${query}_cookies = response->cookies(response);"
            echo "      cad_cgi_cookie_t *${query}_${cookie_write}_websid = new_cad_cgi_cookie(this->memory, \"WEBSID\");"
            echo "      ${query}_${cookie_write}_websid->set_value(${query}_${cookie_write}_websid, (char*)${query}_reply->${cookie_write}(${query}_reply));"
            echo "      ${query}_${cookie_write}_websid->set_max_age(${query}_${cookie_write}_websid, 900);"
            echo "      ${query}_${cookie_write}_websid->set_flag(${query}_${cookie_write}_websid, Cookie_secure | Cookie_http_only);"
            echo "      ${query}_cookies->set(${query}_cookies, ${query}_${cookie_write}_websid);"
        elif [ "$cookie_read" != null ]; then
            echo "      log_debug(this->log, \"web\", \"Updating cookie: ${cookie_read}\");"
            echo "      cad_cgi_cookies_t *${query}_cookies = response->cookies(response);"
            echo "      cad_cgi_cookie_t *${query}_${cookie_read}_websid = ${query}_cookies->get(${query}_cookies, \"WEBSID\");"
            echo "      ${query}_${cookie_read}_websid->set_max_age(${query}_${cookie_read}_websid, 900);"
            echo "      ${query}_${cookie_read}_websid->set_flag(${query}_${cookie_read}_websid, Cookie_secure | Cookie_http_only);"
        fi
        echo "   } else {"
        echo "      set_response_string(this, response, 500, \"Invalid reply from server\");"
        echo "   }"
    } >> $write_file
}

# ----------------------------------------------------------------
# MAIN

for_all webh webc

exit 0

# ----------------------------------------------------------------
# TEMPLATE

function do_() {
    :
}

function od_() {
    :
}

function url_() {
    local url=$1
}

function lru_() {
    local url=$1
}

function query_() {
    local query="$1"
    local params="$2"
    local cookie_read="$3"
}

function reply_() {
    local reply="$1"
    local extra="$2"
    local cookie_read="$3"
    local cookie_write="$4"
    local template="$5"
}
