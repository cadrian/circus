set -e
find test -name \*.c | while read test; do
    redo-ifchange ${test%.c}.test
done
