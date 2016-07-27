#!/usr/bin/env bash

set -e

ROOTDIR=$(readlink -f $1)
shift

function deps_of() {
    local obj=$1
    egrep -o  "$ROOTDIR/inc/circus_[^[:space:]]+\.h" ${obj%.o}.d |
        sed -r 's!^'"$ROOTDIR"'/inc/circus_([^.]+)\.h$!\1!' |
        while read header; do
            find $ROOTDIR/exe -name gen -prune -o -name $header\*.c -exec egrep -l '^\#include <circus_'$header'\.h>$' {} + || true
        done |
        sed 's!\.c$!.o!' |
        fgrep -v "$obj" |
        sort -u
}

function all_deps() {
    local obj=$(readlink -f $1)
    test -f $obj.d0 || {
        deps_of $obj > $obj.d0
        touch $obj.d1
        while ! diff -q $obj.d0 $obj.d1 >/dev/null; do
            while read o; do
                all_deps $(readlink -f $o)
            done < $obj.d0 > $obj.d1
            cat $obj.d0 >> $obj.d1
            sort -u $obj.d1 > $obj.d0
        done
        cat $obj.d0
    }
}

find $ROOTDIR \( -name \*.d0 -o -name \*.d1 \) -exec rm -f {} +
for obj in "$@"; do
    all_deps $obj
done |
    sort -u
find $ROOTDIR \( -name \*.d0 -o -name \*.d1 \) -exec rm -f {} +
