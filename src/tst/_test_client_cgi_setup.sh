HOME=$(pwd)
DIR=${TESTDIR:-$(mktemp --tmpdir -d test_cgi.XXXXXX)}

CONF=$DIR/conf
ROOT=$DIR/root
LOG=$DIR/log
RUN=$DIR/run

mkdir -p $CONF $ROOT $LOG $RUN

cat > $CONF/lighttpd.conf <<EOF
server.document-root = "$ROOT"
server.port = 8888
server.tag = "test_cgi"
server.modules = ("mod_cgi","mod_auth","mod_accesslog")

auth.backend = "plain"
auth.backend.plain.userfile = "$CONF/users"
auth.require = ( "/" => (
        "method" => "basic",
        "realm" => "Password protected area",
        "require" => "user=test"
    )
)

mimetype.assign = (
    ".html" => "text/html",
    ".txt"  => "text/plain",
    ".jpg"  => "image/jpeg",
    ".png"  => "image/png",
    ""      => "application/octet-stream"
)

static-file.exclude-extensions = ( ".fcgi", ".php", ".rb", "~", ".inc", ".cgi" )
index-file.names = ( "index.html" )

server.errorlog = "$LOG/error.log"
server.breakagelog = "$LOG/breakage.log"
accesslog.filename = "$LOG/access.log"

\$HTTP["url"] =~ "^/test_cgi\.cgi" {
    cgi.assign = ( ".cgi" => "$(which bash)" )
}
EOF

cat > $CONF/users <<EOF
test:pwd
EOF

cat > $ROOT/index.html <<EOF
<html>
    <head>
        <meta http-equiv="refresh" content="0; url=/test_cgi.cgi">
    </head>
</html>
EOF

cat >$ROOT/test_cgi.cgi <<EOF
#!/bin/dash
export PATH=/bin:/usr/bin
export HOME=$DIR
export XDG_CACHE_HOME=$RUN
export XDG_RUNTIME_DIR=$RUN
export XDG_DATA_HOME=$RUN
export XDG_CONFIG_HOME=$CONF
export XDG_DATA_DIRS=$RUN
export XDG_CONFIG_DIRS=$RUN
{
    env | sort
    echo
    echo script is $HOME/exe/main/client_cgi.dbg.exe
} > $LOG/env.log
valgrind --verbose --leak-check=full --track-origins=yes --trace-children=yes --log-file=$LOG/valgrind.log $HOME/exe/main/client_cgi.dbg.exe
#exec $HOME/exe/main/client_cgi.dbg.exe
EOF

$HOME/exe/main/server.dbg.exe&
server_pid=$!

(
    cd $DIR
    exec /usr/sbin/lighttpd -D -f $CONF/lighttpd.conf
) &
lighttpd_pid=$!

sleep 2
