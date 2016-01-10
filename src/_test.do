set -e
find tst -name \*.c | while read test; do
    redo-ifchange ${test%.c}.test
done
