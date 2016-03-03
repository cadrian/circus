set -e
redo-ifchange web.json
redo-ifchange web.sh
./web.sh >$3
st=$?
exit $st
