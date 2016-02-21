set -e
find tst -name test_\*.c -a -not -name test_server_\*.c | while read test; do
    redo-ifchange ${test%.c}.test
done
find tst -name test_server_\*.c | while read test; do
    redo-ifchange ${test%.c}.test
done
