set -e
cd src
find test -name \*.c | while read test; do
    redo ${test%.c}.test
done
