set -e
cd src
find test -name \*.c | while read test; do
    redo-ifchange ${test%.c}.test
done
