set -e
find tst -name test_\*.c | while read test; do
    redo-ifchange ${test%.c}.test
done
