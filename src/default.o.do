set -e
out=${2%.dbg}
redo-ifchange $out.c
gcc -std=gnu11 -Wall -Wextra -Werror -Wshadow -Wstrict-overflow -fno-strict-aliasing -Wno-missing-field-initializers ${CFLAGS--O2} -fsanitize=undefined -I $(pwd)/inc -MD -MF $out.d -c -o $3 $out.c
read DEPS <$out.d
redo-ifchange ${DEPS#*:}
