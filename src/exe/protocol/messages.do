set -e
redo-ifchange messages.json
./messages.sh >$3
st=$?
exit $st
