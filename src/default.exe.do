set -e
#set -x

redo-ifchange exe/protocol/messages

function deps_of() {
    egrep -o  "$(pwd)/inc/circus_[^[:space:]]+\.h" ${1%.o}.d |
        sed -r 's!^'"$(pwd)"'/inc/circus_([^.]+)\.h$!\1!' |
        while read header; do
            find exe -name gen -prune -o -name $header\*.c -exec egrep -l '^\#include <circus_'$header'\.h>$' {} + || true
        done |
        sed 's!\.c$!.o!'
}

function rebuild_deps() {
    # Try to recursively rebuild all deps until fix point
    rm -f $1.dep[s01]
    {
        # The seed is the target itself
        echo $1.o
        # Force circus.o because deps_of will not find it
        echo exe/circus.o
        # Be sure to add all the program-specific modules
        for f in exe/$(basename $1)/*.c; do
            test -r "$f" && echo ${f%.c}.o
        done
    } > $1.deps
    # We stop when the list of deps does not change anymore (fix point)
    until diff -q $1.deps $1.dep0 >/dev/null 2>&1; do
        mv $1.deps $1.dep0
        touch $1.dep1
        while read o; do
            echo $o >> $1.dep1
            redo-ifchange $o
            deps_of $o >> $1.dep1
        done < $1.dep0
        cat $1.dep1 | sort -u > $1.deps
        rm $1.dep1
    done
    rm $1.dep0
    cat $1.deps | grep -v "^$1.o$" | awk '{printf("%s ", $0)} END {printf("\n")}'
    rm $1.deps
}

DEPS=$(rebuild_deps $2)
export LD_FLAGS="$(echo ${LD_FLAGS-""} | sed 's/ /\n/g' | awk 'BEGIN {a=""} {a=sprintf("%s-Wl,%s ", a, $0)} END {print a}')"

libs="-lcad -lyacjp -luv -lzmq"
case $(basename $2) in
    server)
        libs="$libs -lsqlite3 -lgcrypt"
        ;;
    test_server*)
        redo-ifchange exe/config/xdg.o
        libs="$(dirname $2)/_test_server.o exe/config/xdg.o $libs -lsqlite3 -lgcrypt"
        ;;
esac

gcc -std=gnu11 -Wall -Wextra -Wshadow -Wstrict-overflow -fno-strict-aliasing -Wno-missing-field-initializers $LD_FLAGS -fsanitize=undefined -o $3 $2.o $DEPS $libs
