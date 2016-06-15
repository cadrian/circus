exe=${exe-.dbg.exe}

export ROOT=${ROOT-$(cd $(dirname $(readlink -f $0))/..; pwd)}
DIR=${TESTDIR:-$(mktemp --tmpdir -d test_cgi.XXXXXX)}

rm -rf $ROOT/.local

CONF=$DIR/conf
WEB=$DIR/web
LOG=$DIR/log
RUN=$DIR/run

mkdir -p $CONF $WEB $LOG $RUN

export XDG_CACHE_HOME=$RUN
export XDG_RUNTIME_DIR=$RUN
export XDG_DATA_HOME=$RUN
export XDG_CONFIG_HOME=$CONF
export XDG_DATA_DIRS=$RUN
export XDG_CONFIG_DIRS=$RUN

cat > $CONF/lighttpd.conf <<EOF
server.document-root = "$WEB"
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

cat > $WEB/index.html <<EOF
<html>
    <head>
        <meta http-equiv="refresh" content="0; url=/test_cgi.cgi">
    </head>
</html>
EOF

echo 0 >$base.count
cat >$WEB/test_cgi.cgi <<EOF
#!/bin/dash
export PATH=/bin:/usr/bin
export HOME=$DIR
export XDG_CACHE_HOME=$XDG_CACHE_HOME
export XDG_RUNTIME_DIR=$XDG_RUNTIME_DIR
export XDG_DATA_HOME=$XDG_DATA_HOME
export XDG_CONFIG_HOME=$XDG_CONFIG_HOME
export XDG_DATA_DIRS=$XDG_DATA_DIRS
export XDG_CONFIG_DIRS=$XDG_CONFIG_DIRS
i=\$((\$(< $base.count) + 1))
echo \$i > $base.count
ext=.\$(printf "%02d" \$i)
{
    env | sort
} > $LOG/env\$ext.log
#$ROOT/exe/main/client_cgi$exe |
tee $base\$ext.in |
    valgrind --verbose --leak-check=full --track-origins=yes --trace-children=yes --log-file=$LOG/valgrind\$ext.log $ROOT/exe/main/client_cgi$exe |
    tee $base\$ext.out
EOF

mkdir -p $RUN/circus $CONF/templates
rm -f $base.client_log $base.server_log

cat > $RUN/circus/cgi.conf <<EOF
{
    "cgi": {
        "templates_path": "$CONF/templates",
        "secure": "No: Test"
    },
    "log": {
        "level": "pii",
        "filename": "$base.client_log"
    }
}
EOF

cat > $RUN/circus/server.conf <<EOF
{
    "vault": {
        "filename": "$base.vault"
    },
    "log": {
        "level": "pii",
        "filename": "$base.server_log"
    }
}
EOF

(
    export PATH=/bin:/usr/bin
    export HOME=$DIR
    exec $ROOT/exe/main/server$exe --install admin password
) >$base.install_out 2>$base.install_err

(
    export PATH=/bin:/usr/bin
    export HOME=$DIR
    exec $ROOT/exe/main/server$exe
) >$base.server_out 2>$base.server_err &
server_pid=$!

(
    cd $DIR
    exec /usr/sbin/lighttpd -D -f $CONF/lighttpd.conf
) &
lighttpd_pid=$!

sleep 2

ps -p $server_pid,$lighttpd_pid -F > $base.ps
