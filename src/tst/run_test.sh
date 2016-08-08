#!/usr/bin/env bash

set -e

exe=$1
out=$2

echo pwd=$(pwd)
echo exe=$exe
echo out=$out

exec >$out

echo -n start:
date

lognew=${exe%.exe}.log.new
logref=${exe%.exe}.log.ref
logerr=${exe%.exe}.log.err

if [[ ${LIBUV_DIR-x} != x ]]; then
    export LD_LIBRARY_PATH=$LIBUV_DIR/lib:${LD_LIBRARY_PATH-}
fi

if [ -f ${exe%.exe}.c ]; then
    (
        cd $(dirname $exe)
        exe=$(basename $exe)

        case $exe in
            test_server*)
                export XDG_DATA_HOME=$(pwd)/${exe%.exe}-conf.d
                mkdir -p $XDG_DATA_HOME/circus
                cat > $XDG_DATA_HOME/circus/server.conf <<EOF
{
    "vault": {
        "filename": "vault"
    },
    "log": {
        "level": "pii",
        "filename": "${exe%.exe}-server.log"
    }
}
EOF
                ;;
        esac

        if [ -f ${exe%.exe}.sh ]; then
            exec ${exe%.exe}.sh $exe
        else
            # DETAILED VALGRIND REPORTS:
            #exec valgrind --leak-check=full --trace-children=yes --read-var-info=yes --fair-sched=no --track-origins=yes --malloc-fill=0A --free-fill=DE \
            #     --xml=yes --xml-file=${exe%.exe}.log.valgrind.xml --log-file=${exe%.exe}.log.valgrind $exe
            #
            # CONCISE VALGRIND REPORTS:
            #exec valgrind --trace-children=yes --log-file=${exe%.exe}.log.valgrind $exe
            #
            # RAW EXECUTION: (fastest)
            exec $exe
            #
        fi
    ) >$lognew 2>$logerr || {
        echo "**** Exited with status $?" >>$lognew
        cat $lognew >&2
        exit 1
    }
elif [ -f ${exe%.exe}.sh ]; then
    ${exe%.exe}.sh >$lognew 2>$logerr || {
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

echo -n end:
date
