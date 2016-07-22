set -e

export LD_FLAGS="--wrap=mlock --wrap=munlock --wrap=gcry_randomize --wrap=gcry_create_nonce --wrap=now"

case $(basename $2) in
    test_server*)
        redo-ifchange $(dirname $2)/../exe/main/server.dbg.exe
        redo-ifchange $(dirname $2)/_test_server.dbg.o
        ;;
    test_client*)
        redo-ifchange $(dirname $2)/../exe/main/server.dbg.exe
        redo-ifchange $(dirname $2)/../exe/main/client_cgi.dbg.exe
        ;;
esac

lognew=$2.log.new
logref=$2.log.ref
logerr=$2.log.err

if [[ ${LIBUV_DIR-x} != x ]]; then
    export LD_LIBRARY_PATH=$LIBUV_DIR/lib:${LD_LIBRARY_PATH-}
fi

if [ -f ${1%.test}.c ]; then
    exe=$2.dbg.exe
    redo-ifchange $exe
    (
        cd $(dirname $exe)

        export XDG_DATA_HOME=$(basename $2)-conf.d
        mkdir -p $XDG_DATA_HOME/circus
        cat > $XDG_DATA_HOME/circus/server.conf <<EOF
{
    "vault": {
        "filename": "vault"
    },
    "log": {
        "level": "pii",
        "filename": "$(basename $2)-server.log"
    }
}
EOF

        # DETAILED VALGRIND REPORTS:
        #exec valgrind --leak-check=full --trace-children=yes --read-var-info=yes --fair-sched=no --track-origins=yes --malloc-fill=0A --free-fill=DE \
        #     --xml=yes --xml-file=$(basename $2).log.valgrind.xml --log-file=$(basename $2).log.valgrind $(basename $exe)
        #
        # CONCISE VALGRIND REPORTS:
        #exec valgrind --trace-children=yes --log-file=$(basename $2).log.valgrind $(basename $exe)
        #
        # RAW EXECUTION: (fastest)
        exec $(basename $exe)
        #
    ) >$lognew 2>$logerr || {
        echo "**** Exited with status $?" >>$lognew
        cat $lognew >&2
        exit 1
    }
elif [ -f ${1%.test}.sh ]; then
    redo-ifchange ${1%.test}.sh
    ${1%.test}.sh >$lognew 2>$logerr || {
        echo "**** Exited with status $?" >>$lognew
        cat $lognew >&2
        exit 1
    }
fi

if [ -r $logref ]; then
    diff -u $logref $lognew >&2
else
    echo "There is no $logref file. Please check and copy $lognew" >&2
    exit 1
fi
