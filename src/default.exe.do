set -e
redo-ifchange $2.o
DEPS=$(egrep -o  "$(pwd)/main/[^[:space:]]+\.h" $2.d |
              sed -r 's!'^"$(pwd)"'/main/!!;s!\.h$!!' |
              while read header; do
                  grep -l '^#include "'$(basename $header)'.h"$' main/$header*.c
              done |
              sed 's!\.c$!.o!'
    )
redo-ifchange $DEPS
LD_FLAGS=""
if [[ "${LDFLAGS-x}" != x ]]; then
    LD_FLAGS="$(echo $LDFLAGS | sed 's/ /\n/g' | awk 'BEGIN {a=""} {a=sprintf("%s-Wl,%s ", a, $0)} END {print a}')"
fi
echo "LD_FLAGS=$LD_FLAGS" >&2
gcc -Wall -Werror $LD_FLAGS -O2 -o $3 $2.o $DEPS -lcad -luv
