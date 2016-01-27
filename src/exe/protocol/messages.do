set -e
redo-ifchange messages.json
redo-ifchange messages.sh
./messages.sh >$3
st=$?
exit $st
