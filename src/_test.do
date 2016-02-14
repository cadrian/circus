set -e
export LD_FLAGS="--wrap=mlock --wrap=munlock --wrap=gcry_randomize"
find tst -name test_\*.c | while read test; do
    redo-ifchange ${test%.c}.test
done
