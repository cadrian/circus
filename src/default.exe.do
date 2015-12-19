redo-ifchange $2.o
DEPS=$(egrep -o  "$(pwd)/main/[^[:space:]]+\.h" $2.d |
              sed -r 's!'^"$(pwd)"'/main/!!;s!\.h$!!' |
              while read header; do
                  grep -l '^#include "'$(basename $header)'.h"$' main/$header*.c
              done |
              sed 's!\.c$!.o!'
    )
redo-ifchange $DEPS
gcc -Wall -Werror -O2 -o $3 $2.o $DEPS -lcad -luv
