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
# Copyright © 2015-2017 Cyril Adrian <cyril.adrian@gmail.com>

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
function jq_msg() {
    for item in $(eval echo $($JQ "$@ | @sh" < ../protocol/messages.json)); do
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
    local method;
    local url;
    local action;

    for fn in "$@"; do
        do_$fn
    done

    method=get
    echo METHOD: "$method"
    jq '.["'$method'"] | keys' | while read url; do
        echo URL: "$url"
        for fn in "$@"; do
            url_$fn "$method" "$url"
        done

        action=""

        local query=$(jq '.["'$method'"]["'$url'"]["query"]')
        local reply=$(jq '.["'$method'"]["'$url'"]["reply"]')
        local params=$(jq '.["'$method'"]["'$url'"]["params"]')
        local extra=$(jq '.["'$method'"]["'$url'"]["extra"]')
        local cookie_read=$(jq '.["'$method'"]["'$url'"]["cookie"]["read"]')
        local cookie_write=$(jq '.["'$method'"]["'$url'"]["cookie"]["write"]')
        local template=$(jq '.["'$method'"]["'$url'"]["template"]')

        echo "query: $query"
        echo "reply: $reply"
        echo "params: $params"
        echo "extra: $extra"
        echo "cookie: read ${cookie_read} - write ${cookie_write}"
        echo "template: $template"

        query_$fn "$method" "$action" "$query" "$params" "$cookie_read" ""
        reply_$fn "$method" "$action" "$reply" "$extra" "$cookie_read" "$cookie_write" "$template" ""

        for fn in "$@"; do
            lru_$fn "$method" "$url"
        done
    done

    method=post
    echo METHOD: "$method"
    jq '.["'$method'"] | keys' | while read url; do
        echo URL: "$url"
        for fn in "$@"; do
            url_$fn "$method" "$url"
        done
        jq '.["'$method'"]["'$url'"] | keys' | while read action; do
            echo ACTION: "$action"

            local query=$(jq '.["'$method'"]["'$url'"]["'$action'"]["query"]')
            local reply=$(jq '.["'$method'"]["'$url'"]["'$action'"]["reply"]')
            local params=$(jq '.["'$method'"]["'$url'"]["'$action'"]["params"]')
            local extra=$(jq '.["'$method'"]["'$url'"]["'$action'"]["extra"]')
            local cookie_read=$(jq '.["'$method'"]["'$url'"]["'$action'"]["cookie"]["read"]')
            local cookie_write=$(jq '.["'$method'"]["'$url'"]["'$action'"]["cookie"]["write"]')
            local template=$(jq '.["'$method'"]["'$url'"]["'$action'"]["template"]')
            local redirect=$(jq '.["'$method'"]["'$url'"]["'$action'"]["redirect"]')

            echo "query: $query"
            echo "reply: $reply"
            echo "params: $params"
            echo "extra: $extra"
            echo "cookie: read ${cookie_read} - write ${cookie_write}"
            echo "template: $template"

            query_$fn "$method" "$action" "$query" "$params" "$cookie_read" "$redirect"
            reply_$fn "$method" "$action" "$reply" "$extra" "$cookie_read" "$cookie_write" "$template" "$redirect"
        done
        for fn in "$@"; do
            lru_$fn "$method" "$url"
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
        echo 'void get_read(impl_cgi_t *this, cad_cgi_response_t *response);'
        echo 'void get_write(impl_cgi_t *this, cad_cgi_response_t *response);'
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
    local method="$1"
    local url="$2"
}

function lru_webh() {
    local method="$1"
    local url="$2"
}

function query_webh() {
    local method="$1"
    local action="$2"
    local query="$3"
    local params="$4"
    local cookie_read="$5"
    local redirect="$6"
}

function reply_webh() {
    local method="$1"
    local action="$2"
    local reply="$3"
    local extra="$4"
    local cookie_read="$5"
    local cookie_write="$6"
    local template="$7"
    local redirect="$8"
}

# ----------------------------------------------------------------
# WEB CODE

function do_webc() {
    local file=$(init_file web.c)
    {
        echo '#include <alloca.h>'
        echo '#include <string.h>'
        echo
        echo '#include <circus_message_impl.h>'
        echo '#include "web.h"'
        echo
        echo 'void get_read(impl_cgi_t *this, cad_cgi_response_t *response) {'
        echo '   assert(!this->ready);'
        echo '   circus_message_t *message = NULL;'
        echo '   cad_cgi_meta_t *meta = response->meta_variables(response);'
        echo '   const char *path = meta->path_info(meta);'
        echo '   cad_hash_t *form = meta->query_string(meta);'
        echo '   log_debug(this->log, "GET path: %s", path);'
        echo '#include "get_read.c"'
        echo '   if (!this->ready) {'
        echo '      if (message == NULL) {'
        echo '         set_response_string(this, response, 401, "Invalid path");'
        echo '      } else {'
        echo '         this->automaton->set_state(this->automaton, State_write_to_server, message);'
        echo '      }'
        echo '   }'
        echo '}'
        echo
        echo 'void get_write(impl_cgi_t *this, cad_cgi_response_t *response) {'
        echo '   circus_message_t *message = this->automaton->message(this->automaton);'
        echo '   if (!this->ready) {'
        echo '      const char *error = message == NULL ? "no message" : message->error(message);'
        echo '      if (strlen(error) == 0) {'
        echo '         cad_cgi_meta_t *meta = response->meta_variables(response);'
        echo '         const char *path = meta->path_info(meta);'
        echo '         log_debug(this->log, "GET path: %s - message type: %s - command: %s", path, message->type(message), message->command(message));'
        echo '#include "get_write.c"'
        echo '      } else {'
        echo '         log_error(this->log, "error: %s", error);'
        echo '         set_response_string(this, response, 403, "Access denied");'
        echo '      }'
        echo '   }'
        echo '}'
        echo
        echo 'void post_read(impl_cgi_t *this, cad_cgi_response_t *response) {'
        echo '   assert(!this->ready);'
        echo '   circus_message_t *message = NULL;'
        echo '   cad_cgi_meta_t *meta = response->meta_variables(response);'
        echo '   const char *path = meta->path_info(meta);'
        echo '   cad_hash_t *form = meta->input_as_form(meta);'
        echo '   assert(form != NULL);'
        echo '   const char *action = form->get(form, "action");'
        echo '   log_debug(this->log, "POST path: %s - action: %s", path, action);'
        echo '#include "post_read.c"'
        echo '   if (!this->ready) {'
        echo '      if (message == NULL) {'
        echo '         set_response_string(this, response, 401, "Invalid path");'
        echo '      } else if (action == NULL) {'
        echo '         set_response_string(this, response, 401, "Invalid action");'
        echo '      } else {'
        echo '         this->automaton->set_state(this->automaton, State_write_to_server, message);'
        echo '      }'
        echo '   }'
        echo '}'
        echo
        echo 'void post_write(impl_cgi_t *this, cad_cgi_response_t *response) {'
        echo '   circus_message_t *message = this->automaton->message(this->automaton);'
        echo '   if (!this->ready) {'
        echo '      const char *error = message == NULL ? "no message" : message->error(message);'
        echo '      if (strlen(error) == 0) {'
        echo '         cad_cgi_meta_t *meta = response->meta_variables(response);'
        echo '         const char *path = meta->path_info(meta);'
        echo '         log_debug(this->log, "POST path: %s - message type: %s - command: %s", path, message->type(message), message->command(message));'
        echo '#include "post_write.c"'
        echo '      } else {'
        echo '         log_error(this->log, "error: %s", error);'
        echo '         set_response_string(this, response, 403, "Access denied");'
        echo '      }'
        echo '   }'
        echo '}'
    } >> $file
}

function od_webc() {
    local read_file=$(init_file get_read.c)
    {
        echo '{'
        echo '   set_response_string(this, response, 404, "Not found");'
        echo '}'
    } >> $read_file
    local write_file=$(init_file get_write.c)
    {
        echo '{'
        echo '   set_response_string(this, response, 404, "Not found");'
        echo '}'
    } >> $write_file
    local read_file=$(init_file post_read.c)
    {
        echo '{'
        echo '   set_response_string(this, response, 404, "Not found");'
        echo '}'
    } >> $read_file
    local write_file=$(init_file post_write.c)
    {
        echo '{'
        echo '   set_response_string(this, response, 404, "Not found");'
        echo '}'
    } >> $write_file
}

function url_webc() {
    local method="$1"
    local url="$2"
    local read_file=$(init_file ${method}_read.c)
    {
        echo 'if (!strcmp(path, "'"$url"'")) {'
        echo -n '   '
    } >> $read_file
    local write_file=$(init_file ${method}_write.c)
    {
        echo 'if (!strcmp(path, "'"$url"'")) {'
        echo -n "   "
    } >> $write_file
}

lru_webc_count=0
function lru_webc() {
    local method="$1"
    local url="$2"
    local read_file=$(init_file ${method}_read.c)
    lru_webc_count=$(($lru_webc_count + 1))
    {
        if [ $method == post ]; then
            echo '{'
            echo '      set_response_string(this, response, 401, "Invalid action");'
            echo '   }'
        fi
        echo -n '} else '
    } >> $read_file
    local write_file=$(init_file ${method}_write.c)
    {
        echo '{'
        echo "      char *invalid_response$lru_webc_count = szprintf(this->memory, NULL, \"Invalid reply from server (type: %s; command: %s; path: %s)\", message->type(message), message->command(message), path);"
        echo "      set_response_string(this, response, 500, invalid_response$lru_webc_count);"
        echo "      this->memory.free(invalid_response$lru_webc_count);"
        echo '   }'
        echo -n '} else '
    } >> $write_file
}

function query_webc() {
    local method="$1"
    local action="$2"
    local query="$3"
    local params="$4"
    local cookie_read="$5"
    local redirect="$6"
    local read_file=$(init_file ${method}_read.c)
    {
        if [ $method == post ]; then
            echo 'if (!strcmp(action, "'"$action"'")) {'
        fi
        if [[ "$query" == "null" ]]; then # TODO ouch, hard-coded token! (must be in the template)
            echo "      cad_hash_t *${action}_extra = cad_new_hash(this->memory, cad_hash_strings);"
            echo "      if (form != NULL) {"
            echo "         ${action}_extra->set(${action}_extra, \"token\", form->get(form, \"token\"));"
            echo "      }"
            case $method in
                get)
                    echo "      set_response_template(this, response, 200, \"$template\", ${action}_extra);"
                    ;;
                post)
                    echo "      set_response_redirect(this, response, \"$redirect\", ${action}_extra);"
                    ;;
            esac
        else
            for param in $params; do
                echo "      const char *${query}_$param = form->get(form, \"$param\");"
            done
            if [ "$cookie_read" != null ]; then
                echo "      cad_cgi_cookies_t *${query}_cookies = response->cookies(response);"
                echo "      cad_cgi_cookie_t *${query}_${cookie_read}_websid = ${query}_cookies->get(${query}_cookies, \"WEBSID\");"
                echo "      if (${query}_${cookie_read}_websid != NULL) {"
                echo "         const char *${query}_${cookie_read} = ${query}_${cookie_read}_websid->value(${query}_${cookie_read}_websid);"
                echo -n "   "
            fi
            echo -n "      message = I(new_circus_message_query_$query(this->memory"
            if [ "$cookie_read" != null ]; then
                echo -n ", ${query}_${cookie_read}"
            fi
            for param in $params; do
                echo -n ", ${query}_$param"
            done
            echo '));'
            if [ "$cookie_read" != null ]; then
                echo "      } else {"
                echo "         set_response_string(this, response, 403, \"Not authenticated.\");"
                echo "      }"
            fi
        fi
        if [ $method == post ]; then
            echo -n '   } else '
        fi
    } >> $read_file
}

function reply_webc() {
    local method="$1"
    local action="$2"
    local reply="$3"
    local extra="$4"
    local cookie_read="$5"
    local cookie_write="$6"
    local template="$7"
    local redirect="$8"
    local write_file=$(init_file ${method}_write.c)
    {
        if [[ "$reply" == "null" ]]; then
            echo "if (1) {"
            echo "      crash(); /* unexpected reply from no query */"
        else
            echo "if (!strcmp(message->type(message), \"$reply\") && !strcmp(message->command(message), \"reply\")) {"
            echo "      cad_hash_t *${reply}_extra = cad_new_hash(this->memory, cad_hash_strings);"
            if [[ "$extra" != "null" ]]; then
                echo "      circus_message_reply_${reply}_t *${reply}_reply = (circus_message_reply_${reply}_t*)message;"
                for extrum in $extra; do
                    msg_type=$(jq_msg '.["'$reply'"]["reply"]["'$extrum'"]')
                    case $msg_type in
                        STRING)
                            echo "      log_pii(this->log, \"Setting extra: \\\"$extrum\\\" = %s\", (char*)${reply}_reply->$extrum(${reply}_reply));"
                            echo "      ${reply}_extra->set(${reply}_extra, \"$extrum\", (char*)${reply}_reply->$extrum(${reply}_reply));"
                            ;;
                        STRINGS)
                            echo "      log_pii(this->log, \"Setting extra: \\\"#$extrum\\\" = (%d)\", ${reply}_reply->$extrum(${reply}_reply)->count(${reply}_reply->$extrum(${reply}_reply)));"
                            echo "      ${reply}_extra->set(${reply}_extra, \"#$extrum\", ${reply}_reply->$extrum(${reply}_reply));"
                            ;;
                        *)
                            echo "      // $extrum: type $msg_type not supported"
                            ;;
                    esac
                done
            fi
            case $method in
                get)
                    echo "      set_response_template(this, response, 200, \"$template\", ${reply}_extra);"
                    ;;
                post)
                    echo "      set_response_redirect(this, response, \"$redirect\", ${reply}_extra);"
                    ;;
            esac
            echo "      ${reply}_extra->free(${reply}_extra);"
            if [[ "$extra" != "null" ]]; then
                if [ "$cookie_write" != null ]; then
                    echo "      char *${reply}_${cookie_write}_path = \"/\";"
                    echo "      const char *${reply}_${cookie_write}_script_name = meta->script_name(meta);"
                    echo "      if (${reply}_${cookie_write}_script_name != NULL && strlen(${reply}_${cookie_write}_script_name) > 0) {"
                    echo "         ${reply}_${cookie_write}_path = alloca(strlen(${reply}_${cookie_write}_script_name) + 2);"
                    echo "         sprintf(${reply}_${cookie_write}_path, \"%s/\", ${reply}_${cookie_write}_script_name);"
                    echo "      }"
                    echo "      log_debug(this->log, \"Setting cookie: ${cookie_write} = %s\", (char*)${reply}_reply->${cookie_write}(${reply}_reply));"
                    echo "      log_debug(this->log, \"Cookie path: %s\", ${reply}_${cookie_write}_path);"
                    echo "      cad_cgi_cookies_t *${reply}_cookies = response->cookies(response);"
                    echo "      cad_cgi_cookie_t *${reply}_${cookie_write}_websid = new_cad_cgi_cookie(this->memory, \"WEBSID\");"
                    echo "      ${reply}_${cookie_write}_websid->set_value(${reply}_${cookie_write}_websid, (char*)${reply}_reply->${cookie_write}(${reply}_reply));"
                    echo "      ${reply}_${cookie_write}_websid->set_max_age(${reply}_${cookie_write}_websid, 900);"
                    echo "      ${reply}_${cookie_write}_websid->set_flag(${reply}_${cookie_write}_websid, this->cookie_flag);"
                    echo "      ${reply}_${cookie_write}_websid->set_path(${reply}_${cookie_write}_websid, ${reply}_${cookie_write}_path);"
                    echo "      ${reply}_cookies->set(${reply}_cookies, ${reply}_${cookie_write}_websid);"
                elif [ "$cookie_read" != null ]; then
                    echo "      log_debug(this->log, \"Updating cookie: ${cookie_read}\");"
                    echo "      cad_cgi_cookies_t *${reply}_cookies = response->cookies(response);"
                    echo "      cad_cgi_cookie_t *${reply}_${cookie_read}_websid = ${reply}_cookies->get(${reply}_cookies, \"WEBSID\");"
                    echo "      ${reply}_${cookie_read}_websid->set_max_age(${reply}_${cookie_read}_websid, 900);"
                    echo "      ${reply}_${cookie_read}_websid->set_flag(${reply}_${cookie_read}_websid, this->cookie_flag);"
                fi
            fi
        fi
        echo -n "   } else "
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
    local method="$1"
    local url="$2"
}

function lru_() {
    local method="$1"
    local url="$2"
}

function query_() {
    local method="$1"
    local action="$2"
    local query="$3"
    local params="$4"
    local cookie_read="$5"
    local redirect="$6"
}

function reply_() {
    local method="$1"
    local action="$2"
    local reply="$3"
    local extra="$4"
    local cookie_read="$5"
    local cookie_write="$6"
    local template="$7"
    local redirect="$8"
}
