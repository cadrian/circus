TSTDIR=$(cd $(dirname $(readlink -f $0)); pwd)
cd $TSTDIR
ROOT=$(cd $TSTDIR/../..; pwd)

TESTDIR=$TSTDIR/run/$flavour/$(date +'%Y%m%d-%H%M%S')
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
            export STRIP_EXE=false
            exec make manual_test
        ) || exit 1
        ;;
esac

echo "Test base is $base"

echo "Setting up..."
exe=".rnd.exe"
. $ROOT/tst/client/_test_client_cgi_setup_$flavour.sh

cp -a $ROOT/web/templates/* $CONF/templates/
cp -a $ROOT/web/static/* $CONF/static/
yui-compressor -o $CONF/static/clipboard.min.js /usr/share/javascript/clipboard/clipboard.js

teardown() {
    echo
    KEEPDIR=true
    . $ROOT/tst/client/_test_client_cgi_teardown_$flavour.sh
    exit
}

trap teardown INT TERM QUIT EXIT

echo
echo "URL is http://test:pwd@localhost:8888/test_cgi.cgi"
echo
echo "To exit, just hit ^C."
echo
echo "Ready."
echo

wait
