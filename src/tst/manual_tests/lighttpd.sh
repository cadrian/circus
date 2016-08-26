#!/usr/bin/env bash

#set -e
#set -u

TSTDIR=$(cd $(dirname $(readlink -f $0)); pwd)
cd $TSTDIR
ROOT=$(cd $TSTDIR/../..; pwd)

TESTDIR=$TSTDIR/run/lighttpd/$(date +'%Y%m%d-%H%M%S')
base=$TESTDIR/test
mkdir -p $TESTDIR

case x"$1" in
    x--fast)
        :
        ;;
    *)
        (
            cd $ROOT
            export CFLAGS="-DDEBUG -g"
            export LD_FLAGS_DBG="-Wl,--wrap=mlock -Wl,--wrap=munlock -Wl,--wrap=gcry_randomize_rnd -Wl,--wrap=gcry_create_nonce_rnd"
            # mlock/munlock: we are not root
            # gcry_*: we need valgrind (libgcrypt generates instructions not recognized by Valgrind)
            export STRIP_EXE=false
            exec make manual_test
        ) || exit 1
        ;;
esac

echo "Test base is $base"

echo "Setting up..."
exe=".rnd.exe"
. $ROOT/tst/client/_test_client_cgi_setup_lighttpd.sh

cp -a $ROOT/web/templates/* $CONF/templates/
cp -a $ROOT/web/static/* $CONF/static/

teardown() {
    echo
    kill $lighttpd_pid 2>/dev/null
    kill $server_pid 2>/dev/null
    exit
}

trap teardown INT TERM QUIT EXIT

echo
echo "Circus server pid is $server_pid"
echo "Web server pid is $lighttpd_pid"
echo
echo "URL is http://test:pwd@localhost:8888/test_cgi.cgi"
echo
echo "To exit, just hit ^C."
echo
echo "Ready."
echo

wait
