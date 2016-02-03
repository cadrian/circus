set -e

case $(basename $2) in
    test_server*)
        redo-ifchange $(dirname $2)/../exe/main/server.exe
        redo-ifchange $(dirname $2)/_test_server.o
        ;;
esac

lognew=$2.log.new
logref=$2.log.ref

exe=$2.exe
redo $exe
if [[ ${LIBUV_DIR-x} != x ]]; then
    export LD_LIBRARY_PATH=$LIBUV_DIR/lib:{LD_LIBRARY_PATH-}
fi
(
    cd $(dirname $exe)
    exec $(basename $exe)
) >$lognew || {
    echo "**** Exited with status $?" >>$lognew
    echo "cat $lognew" >&2
    exit 1
}

if [ -r $logref ]; then
    diff -u $logref $lognew >&2
else
    echo "mv $lognew $logref" >&2
    exit 1
fi
