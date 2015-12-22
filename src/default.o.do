set -e
redo-ifchange $2.c
gcc -Wall -Werror ${CFLAGS-} -O2 -I $(pwd)/include -MD -MF $2.d -c -o $3 $2.c
read DEPS <$2.d
redo-ifchange ${DEPS#*:}
