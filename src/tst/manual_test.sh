#!/usr/bin/env bash

#set -e
#set -u

TSTDIR=$(cd $(dirname $(readlink -f $0)); pwd)
cd $TSTDIR
ROOT=$(cd $TSTDIR/..; pwd)

TESTDIR=$TSTDIR/manual_test/$(date +'%Y%m%d-%H%M%S')
base=$TESTDIR/test
mkdir -p $TESTDIR

(
    cd ../..
    export LD_FLAGS="--wrap=mlock --wrap=munlock" # we are not root
    exec redo all
) || exit 1

exe=".exe"
. $TSTDIR/_test_client_cgi_setup.sh

cp -a ../../templates/* $CONF/templates/

teardown() {
    kill $lighttpd_pid 2>/dev/null
    kill $server_pid 2>/dev/null
    exit
}

trap teardown INT TERM QUIT EXIT

echo "Test base is $base"
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
