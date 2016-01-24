set -e
redo-ifchange $2.c
gcc -std=gnu11 -Wall -Wextra -Werror -Wshadow -Wstrict-overflow -fno-strict-aliasing -Wno-missing-field-initializers ${CFLAGS--O2} -fsanitize=undefined -I $(pwd)/inc -MD -MF $2.d -c -o $3 $2.c
read DEPS <$2.d
redo-ifchange ${DEPS#*:}
