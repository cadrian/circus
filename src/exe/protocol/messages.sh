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

# This script is very ugly. It builds the messages C header and
# implementation, heavily relying on the C preprocessor.

# Inclusion order:
#
# - factory.h
#     - msg/$type/impl.h
#         - $msg.h
#     - factory/$type/impl.h
#     - visitor_struct_query.h
#         - visitor_query.h
#     - visitor_struct_reply.h
#         - visitor_reply.h
#
# - factory.c
#     - msg/$type/impl.c
#         - impl.h
#         - $msg.c
#     - factory/$type/impl.c
#         - impl.h
#         - deserialize_$msg.c
#         - new_$msg.c

cd $(dirname $(readlink -f "$0"))

rm -rf gen
mkdir gen

JQ=$(which jq) || {
    echo "jq not installed!!" >&2
    exit 1
}
function jq() {
    $JQ "$@" < messages.json | egrep -o '[0-9A-Za-z_]+'
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

function type_of() {
    local keytype=$1
    case $keytype in
        STRING)
            echo "const char *"
            ;;
        STRINGS)
            echo "cad_array_t *"
            ;;
        BOOLEAN)
            echo "int "
            ;;
    esac
}

function for_all() {
    for fn in "$@"; do
        do_$fn
    done
    jq keys < messages.json | egrep -o '[a-z_]+' | while read type; do
        for fn in "$@"; do
            type_$fn $type
        done
        jq '.["'$type'"] | keys' | while read msg; do
            for fn in "$@"; do
                msg_$fn $type $msg
            done
            {
                jq '.["'$type'"]["'$msg'"] | keys' | while read key; do
                    echo $key $(jq '.["'$type'"]["'$msg'"]["'$key'"]')
                done
            } | while read key keytype; do
                for fn in "$@"; do
                    key_$fn $type $msg $key $keytype
                done
            done
            for fn in "$@"; do
                gsm_$fn $type $msg
            done
        done
        for fn in "$@"; do
            epyt_$fn $type
        done
    done
    for fn in "$@"; do
        od_$fn
    done
}

# ----------------------------------------------------------------
# FACTORY HEADER

function do_factoryh() {
    :
}

function od_factoryh() {
    :
}

function type_factoryh() {
    local type=$1
    local file=$(init_file factory.h)
    {
        echo "#include \"msg/$type/impl.h\""
        echo "#include \"factory/$type/impl.h\""
    } >> $file
}

function epyt_factoryh() {
    local type=$1
}

function msg_factoryh() {
    local type=$1
    local msg=$2
    local file=$(init_file "factory/$type/impl.h")
    {
        echo "__PUBLIC__ circus_message_${msg}_${type}_t *deserialize_circus_message_${msg}_${type}(cad_memory_t memory,json_object_t *object);"
        echo -n "__PUBLIC__ circus_message_${msg}_${type}_t *new_circus_message_${msg}_${type}(cad_memory_t memory, const char *error"
    } >> $file
}

function gsm_factoryh() {
    local type=$1
    local msg=$2
    local file=$(init_file "factory/$type/impl.h")
    {
        echo ");"
    } >> $file
}

function key_factoryh() {
    local type=$1
    local msg=$2
    local key=$3
    local keytype=$4
    local file=$(init_file "factory/$type/impl.h")
    {
        echo -n ", $(type_of $keytype)$key"
    } >> $file
}

# ----------------------------------------------------------------
# MESSAGE HEADER

function do_messageh() {
    :
}

function od_messageh() {
    :
}

function type_messageh() {
    local type=$1
    local file=$(init_file msg/$type/impl.h)
    {
        local TYPE=${type^^?}
        echo "#ifndef __CIRCUS_MESSAGE_${TYPE}_IMPL_H"
        echo "#define __CIRCUS_MESSAGE_${TYPE}_IMPL_H"
    } >> $file
}

function epyt_messageh() {
    local type=$1
    local file=$(init_file msg/$type/impl.h)
    {
        local TYPE=${type^^?}
        echo "#endif /* __CIRCUS_MESSAGE_${TYPE}_IMPL_H */"
    } >> $file
}

function msg_messageh() {
    local type=$1
    local msg=$2
    local file=$(init_file msg/$type/impl.h)
    {
        echo "typedef struct circus_message_${msg}_${type}_s circus_message_${msg}_${type}_t;"
    } >> $file
    local fn_file=$(init_file msg/$type/$msg.h)
    {
        echo "struct circus_message_${msg}_${type}_s {"
        echo "    circus_message_t fn;"
    } >> $fn_file
}

function gsm_messageh() {
    local type=$1
    local msg=$2
    local file=$(init_file msg/$type/impl.h)
    {
        echo "#include \"$msg.h\""
    } >> $file
    local fn_file=$(init_file msg/$type/$msg.h)
    {
        echo "};"
    } >> $fn_file
}

function key_messageh() {
    local type=$1
    local msg=$2
    local key=$3
    local keytype=$4
    local file=$(init_file msg/$type/impl.h)
    {
        echo "typedef $(type_of $keytype)(*circus_message_${msg}_${type}_${key}_fn)(circus_message_${msg}_${type}_t *this);"
    } >> $file
    local fn_file=$(init_file msg/$type/$msg.h)
    {
        echo "    circus_message_${msg}_${type}_${key}_fn $key;"
    } >> $fn_file
}

# ----------------------------------------------------------------
# VISITOR HEADER

function do_visitorh() {
    local visitorh=$(init_file visitor.h)
    {
        :
    } >> $visitorh
    local visitorstructqueryh=$(init_file visitor_struct_query.h)
    {
        echo "#include \"visitor.h\""
        echo "struct circus_message_visitor_query_s {"
    } >> $visitorstructqueryh
    local visitorstructreplyh=$(init_file visitor_struct_reply.h)
    {
        echo "#include \"visitor.h\""
        echo "struct circus_message_visitor_reply_s {"
    } >> $visitorstructreplyh
}

function od_visitorh() {
    local visitorstructqueryh=$(init_file visitor_struct_query.h)
    {
        echo "};"
    } >> $visitorstructqueryh
    local visitorstructreplyh=$(init_file visitor_struct_reply.h)
    {
        echo "};"
    } >> $visitorstructreplyh
    local factoryh=$(init_file factory.h)
    {
        echo "#include \"visitor_struct_query.h\""
        echo "#include \"visitor_struct_reply.h\""
    } >> $factoryh
}

function type_visitorh() {
    local type=$1
}

function epyt_visitorh() {
    local type=$1
}

function msg_visitorh() {
    local type=$1
    local msg=$2
    local visitorh=$(init_file visitor.h)
    {
        echo "typedef void (*circus_message_visitor_${msg}_${type}_fn)(circus_message_visitor_${msg%%_*}_t *this, circus_message_${msg}_${type}_t *visited);"
    } >> $visitorh
    case $msg in
        query*)
            local visitorstructqueryh=$(init_file visitor_struct_query.h)
            {
                echo "    circus_message_visitor_${msg}_${type}_fn visit_${msg}_${type};"
            } >> $visitorstructqueryh
            ;;
        reply*)
            local visitorstructreplyh=$(init_file visitor_struct_reply.h)
            {
                echo "    circus_message_visitor_${msg}_${type}_fn visit_${msg}_${type};"
            } >> $visitorstructreplyh
            ;;
    esac
}

function gsm_visitorh() {
    local type=$1
    local msg=$2
}

function key_visitorh() {
    local type=$1
    local msg=$2
    local key=$3
    local keytype=$4
}

# ----------------------------------------------------------------
# FACTORY CODE

function do_factoryc() {
    local file=$(init_file factory.c)
    {
        echo '#include <json.h>'
    } >> $file
    local deserfile=$(init_file factory_deserialize.c)
    {
        echo "    circus_message_t *result = NULL;"
        echo "    json_string_t *jtype = (json_string_t*)object->get(object, \"type\");"
        echo "    int ntype = jtype->utf8(jtype, \"\", 0) + 1;"
        echo "    char *type = memory.malloc(ntype);"
        echo "    jtype->utf8(jtype, type, ntype);"
    } >> $deserfile
}

function od_factoryc() {
    local file=$(init_file factory.c)
    {
        echo "circus_message_t *deserialize_circus_message(cad_memory_t memory,json_object_t *object) {"
        echo '#include "factory_deserialize.c"'
        echo "}"
    } >> $file
    local deserfile=$(init_file factory_deserialize.c)
    {
        echo "    memory.free(type);"
        echo "    return result;"
    } >> $deserfile
}

function type_factoryc() {
    local type=$1
    local file=$(init_file factory.c)
    {
        echo "#include \"msg/$type/impl.c\""
        echo "#include \"factory/$type/impl.c\""
    } >> $file
    local impl_file=$(init_file msg/$type/impl.c)
    {
        echo "#include \"impl.h\""
    } >> $impl_file
    local factory_file=$(init_file factory/$type/impl.c)
    {
        echo "#include \"impl.h\""
    } >> $factory_file
}

function epyt_factoryc() {
    local type=$1
}

function msg_factoryc() {
    local type=$1
    local msg=$2
    local deserfile=$(init_file factory_deserialize.c)
    {
        echo "    if (result == NULL && !strcmp(type, \"${msg}_${type}\")) {"
        echo "        result = (circus_message_t*)deserialize_circus_message_${msg}_${type}(memory, object);"
        echo "    }"
    } >> $deserfile
    local impl_file=$(init_file msg/$type/impl.c)
    {
        echo "#include \"$msg.c\""
    } >> $impl_file
    local msg_file=$(init_file msg/$type/$msg.c)
    {
        echo "typedef struct ${type}_${msg}_impl_s ${type}_${msg}_impl_t;"
        echo "#include \"$msg.struct.c\""
        echo "static const char *${type}_${msg}_type_impl_fn(${type}_${msg}_impl_t *UNUSED(this)) { return \"$type\"; }"
        echo "static const char *${type}_${msg}_command_impl_fn(${type}_${msg}_impl_t *UNUSED(this)) { return \"$msg\"; }"
        echo "static const char *${type}_${msg}_error_impl_fn(${type}_${msg}_impl_t *this) { return this->error; }"
        echo "static void ${type}_${msg}_accept_impl_fn(${type}_${msg}_impl_t *this, circus_message_visitor_${msg%%_*}_t *visitor) {"
        echo "    visitor->visit_${msg}_${type}(visitor, (circus_message_${msg}_${type}_t*)this);"
        echo "}"
        echo "json_object_t *${type}_${msg}_serialize_impl_fn(${type}_${msg}_impl_t *this) {"
        echo "#include \"$msg.serialize.c\""
        echo "}"
        echo "void ${type}_${msg}_free_impl_fn(${type}_${msg}_impl_t *this) {"
        echo "#include \"$msg.free.c\""
        echo "}"
    } >> $msg_file
    local free_file=$(init_file msg/$type/$msg.free.c)
    local serialize_file=$(init_file msg/$type/$msg.serialize.c)
    {
        echo "    json_object_t *result = json_new_object(this->memory);"
        echo "    json_string_t *jtype = json_new_string(this->memory);"
        echo "    jtype->add_string(jtype, \"%s\", \"${msg}_${type}\");"
        echo "    result->set(result, \"type\", (json_value_t*)jtype);"
        echo "    json_string_t *jerror = json_new_string(this->memory);"
        echo "    jerror->add_string(jerror, \"%s\", this->error);"
        echo "    result->set(result, \"error\", (json_value_t*)jerror);"
    } >> $serialize_file
    local struct_file=$(init_file msg/$type/$msg.struct.c)
    {
        echo "struct ${type}_${msg}_impl_s {"
        echo "    circus_message_${msg}_${type}_t fn;"
        echo "    cad_memory_t memory;"
        echo "    char *error;"
    } >> $struct_file
    local fn_file=$(init_file msg/$type/$msg.fn.c)
    {
        echo "static circus_message_${msg}_${type}_t ${type}_${msg}_impl_fn = {"
        echo "    .fn = {"
        echo "        (circus_message_type_fn)${type}_${msg}_type_impl_fn,"
        echo "        (circus_message_command_fn)${type}_${msg}_command_impl_fn,"
        echo "        (circus_message_error_fn)${type}_${msg}_error_impl_fn,"
        echo "        (circus_message_accept_fn)${type}_${msg}_accept_impl_fn,"
        echo "        (circus_message_serialize_fn)${type}_${msg}_serialize_impl_fn,"
        echo "        (circus_message_free_fn)${type}_${msg}_free_impl_fn,"
        echo "    },"
    } >> $fn_file
    local factory_file=$(init_file factory/$type/impl.c)
    {
        echo "circus_message_${msg}_${type}_t *deserialize_circus_message_${msg}_${type}(cad_memory_t memory,json_object_t *object) {"
        echo "#include \"deserialize_$msg.c\""
        echo "}"
        echo -n "__PUBLIC__ circus_message_${msg}_${type}_t *new_circus_message_${msg}_${type}(cad_memory_t memory, const char *error"
    } >> $factory_file
    local new_file=$(init_file factory/$type/new_$msg.c)
    {
        echo "    ${type}_${msg}_impl_t *result = memory.malloc(sizeof(${type}_${msg}_impl_t));"
        echo "    if (result) {"
        echo "        result->fn = ${type}_${msg}_impl_fn;"
        echo "        result->memory = memory;"
        echo "        result->error = dup_string(memory, error);"
    } >> $new_file
    local deserialize_file=$(init_file factory/$type/deserialize_$msg.c)
    {
        echo "    ${type}_${msg}_impl_t *result = memory.malloc(sizeof(${type}_${msg}_impl_t));"
        echo "    if (result) {"
        echo "        result->fn = ${type}_${msg}_impl_fn;"
        echo "        result->memory = memory;"
        echo "        json_string_t *jserror = (json_string_t*)object->get(object, \"error\");"
        echo "        result->error = json_string(memory, jserror);"
    } >> $deserialize_file
}

function gsm_factoryc() {
    local type=$1
    local msg=$2
    local msg_file=$(init_file msg/$type/$msg.c)
    {
        echo "#include \"$msg.fn.c\""
    } >> $msg_file
    local struct_file=$(init_file msg/$type/$msg.struct.c)
    {
        echo "};"
    } >> $struct_file
    local free_file=$(init_file msg/$type/$msg.free.c)
    {
        echo "this->memory.free(this);"
    } >> $free_file
    local serialize_file=$(init_file msg/$type/$msg.serialize.c)
    {
        echo "    return result;"
    } >> $serialize_file
    local fn_file=$(init_file msg/$type/$msg.fn.c)
    {
        echo "};"
    } >> $fn_file
    local factory_file=$(init_file factory/$type/impl.c)
    {
        echo ") {"
        echo "#include \"new_$msg.c\""
        echo "}"
    } >> $factory_file
    local new_file=$(init_file factory/$type/new_$msg.c)
    {
        echo "    }"
        echo "    return I(result);"
    } >> $new_file
    local deserialize_file=$(init_file factory/$type/deserialize_$msg.c)
    {
        echo "    }"
        echo "    return I(result);"
    } >> $deserialize_file
}

function key_factoryc() {
    local type=$1
    local msg=$2
    local key=$3
    local keytype=$4
    local msg_file=$(init_file msg/$type/$msg.c)
    {
        echo "$(type_of $keytype)impl_${type}_${msg}_${key}_fn(${type}_${msg}_impl_t *this) {"
        echo "    return this->$key;"
        echo "}"
    } >> $msg_file
    local struct_file=$(init_file msg/$type/$msg.struct.c)
    {
        echo "    $(type_of $keytype)$key;"
    } >> $struct_file
    local free_file=$(init_file msg/$type/$msg.free.c)
    {
        case $keytype in
            STRING)
                echo "    this->memory.free((char*)this->$key);"
                ;;
            STRINGS)
                echo "    int i$key, n$key = this->$key->count(this->$key);"
                echo "    for (i$key = 0; i$key < n$key; i$key++) {"
                echo "        this->memory.free(this->$key->get(this->$key, i$key));"
                echo "    }"
                echo "    this->$key->free(this->$key);"
                ;;
        esac
    } >> $free_file
    local serialize_file=$(init_file msg/$type/$msg.serialize.c)
    {
        case $keytype in
            STRING)
                echo "    json_string_t *js$key = json_new_string(this->memory);"
                echo "    js$key->add_string(js$key, \"%s\", this->$key);"
                echo "    result->set(result, \"$key\", (json_value_t*)js$key);"
                ;;
            STRINGS)
                echo "    json_array_t *ja$key = json_new_array(this->memory);"
                echo "    int i$key, n$key = this->$key->count(this->$key);"
                echo "    json_string_t *js$key;"
                echo "    char *sz$key;"
                echo "    for (i$key = 0; i$key < n$key; i$key++) {"
                echo "        sz$key = this->$key->get(this->$key, i$key);"
                echo "        js$key = json_new_string(this->memory);"
                echo "        js$key->add_string(js$key, \"%s\", sz$key);"
                echo "        ja$key->set(ja$key, i$key, (json_value_t*)js$key);"
                echo "    }"
                echo "    result->set(result, \"$key\", (json_value_t*)ja$key);"
                ;;
            BOOLEAN)
                echo "    json_const_t *jb$key = json_const(this->$key ? json_true : json_false);"
                echo "    result->set(result, \"$key\", (json_value_t*)jb$key);"
                ;;
        esac
    } >> $serialize_file
    local fn_file=$(init_file msg/$type/$msg.fn.c)
    {
        echo "    .${key} = (circus_message_${msg}_${type}_${key}_fn)impl_${type}_${msg}_${key}_fn,"
    } >> $fn_file
    local factory_file=$(init_file factory/$type/impl.c)
    {
        echo -n ", $(type_of $keytype)$key"
    } >> $factory_file
    local deserialize_file=$(init_file factory/$type/deserialize_$msg.c)
    {
        case $keytype in
            STRING)
                echo "        json_string_t *js$key = (json_string_t*)object->get(object, \"$key\");"
                echo "        result->$key = json_string(memory, js$key);"
                ;;
            STRINGS)
                echo "        json_array_t *ja$key = (json_array_t*)object->get(object, \"$key\");"
                echo "        result->$key = json_strings(memory, ja$key);"
                ;;
            BOOLEAN)
                echo "        json_const_t *jb$key = (json_const_t*)object->get(object, \"$key\");"
                echo "        result->$key = json_boolean(jb$key);"
                ;;
        esac
    } >> $deserialize_file
    local new_file=$(init_file factory/$type/new_$msg.c)
    {
        echo "        result->$key = dup_${keytype,,?}(memory, $key);"
    } >> $new_file
}

# ----------------------------------------------------------------
# MAIN

for_all factoryh messageh visitorh
for_all factoryc

exit 0

# ----------------------------------------------------------------
# TEMPLATE

function do_() {
    :
}

function od_() {
    :
}

function type_() {
    local type=$1
}

function epyt_() {
    local type=$1
}

function msg_() {
    local type=$1
    local msg=$2
}

function gsm_() {
    local type=$1
    local msg=$2
}

function key_() {
    local type=$1
    local msg=$2
    local key=$3
    local keytype=$4
}
