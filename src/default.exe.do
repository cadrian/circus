set -e

redo-ifchange exe/protocol/messages

function deps_of() {
    egrep -o  "$(pwd)/inc/[^[:space:]]+\.h" ${1%.o}.d |
              sed -r 's!'^"$(pwd)"'/inc/!!;s!\.h$!!' |
              while read header; do
                  find exe -name gen -prune -o -name $header\*.c -exec grep -l '^#include "'$header'.h"$' {} +
              done |
              sed 's!\.c$!.o!'
}

function rebuild_deps() {
    # Try to recursively rebuild all deps until fix point
    rm -f $1.dep[s01]
    # The seed is the target itself
    echo $1.o > $1.deps
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
#echo "deps: $DEPS" >&2
LD_FLAGS=""
if [[ "${LDFLAGS-x}" != x ]]; then
    LD_FLAGS="$(echo $LDFLAGS | sed 's/ /\n/g' | awk 'BEGIN {a=""} {a=sprintf("%s-Wl,%s ", a, $0)} END {print a}')"
fi
gcc -Wall -Werror -Wpedantic -Werror -Wshadow -Wstrict-overflow -fno-strict-aliasing -Wno-missing-field-initializers $LD_FLAGS -O2 -g -fsanitize=undefined -o $3 $2.o $DEPS -lcad -lyacjp -luv
