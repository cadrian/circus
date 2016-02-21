set -e

export LD_FLAGS="--wrap=mlock --wrap=munlock --wrap=gcry_randomize --wrap=now"

case $(basename $2) in
    test_server*)
        redo-ifchange $(dirname $2)/../exe/main/server.exe
        redo-ifchange $(dirname $2)/_test_server.o
        ;;
esac

lognew=$2.log.new
logref=$2.log.ref
logerr=$2.log.err

exe=$2.exe
redo-ifchange $exe
if [[ ${LIBUV_DIR-x} != x ]]; then
    export LD_LIBRARY_PATH=$LIBUV_DIR/lib:{LD_LIBRARY_PATH-}
fi
(
    cd $(dirname $exe)
    exec $(basename $exe)
) >$lognew 2>$logerr || {
    echo "**** Exited with status $?" >>$lognew
    cat $lognew >&2
    exit 1
}

if [ -r $logref ]; then
    diff -u $logref $lognew >&2
else
    echo "There is no $logref file. Please check and copy $lognew" >&2
    exit 1
fi
