set -e
for ext in c sh; do
    find tst -name test_\*.$ext -a -not -name test_server_\*.$ext | sort | while read test; do
        redo-ifchange ${test%.$ext}.test
    done
    find tst -name test_server_\*.$ext | sort | while read test; do
        redo-ifchange ${test%.$ext}.test
    done
done
