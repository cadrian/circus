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

fcgiwrap_socket_dir=$(mktemp -t -d fcgi.XXXX)
export fcgiwrap_socket=$fcgiwrap_socket_dir/sock

cat > $CONF/nginx.conf <<EOF
worker_processes auto;
pid $RUN/nginx.pid;
include /etc/nginx/modules-enabled/*.conf;
daemon off;
master_process off;

error_log $LOG/error.log notice;

events {
    worker_connections 4;
}

http {
    sendfile on;
    include /etc/nginx/mime.types;
    default_type application/octet-stream;
    access_log $LOG/access.log;

    server {
        listen 8888;
        listen [::]:8888 ipv6only=on;
        server_name localhost;
        server_tokens off;

        ssl off;

        root "$WEB";

        location ~ ^/test_cgi.cgi(/.*)?\$ {
            fastcgi_pass unix:$fcgiwrap_socket;

            fastcgi_split_path_info "^(.+?\.cgi)(/.*)?\$";

            fastcgi_param  SCRIPT_FILENAME    \$document_root\$fastcgi_script_name;
            fastcgi_param  QUERY_STRING       \$query_string;
            fastcgi_param  REQUEST_METHOD     \$request_method;
            fastcgi_param  CONTENT_TYPE       \$content_type;
            fastcgi_param  CONTENT_LENGTH     \$content_length;

            fastcgi_param  SCRIPT_NAME        \$fastcgi_script_name;
            fastcgi_param  REQUEST_URI        \$request_uri;
            fastcgi_param  DOCUMENT_URI       \$document_uri;
            fastcgi_param  DOCUMENT_ROOT      \$document_root;
            fastcgi_param  SERVER_PROTOCOL    \$server_protocol;
            fastcgi_param  REQUEST_SCHEME     \$scheme;
            fastcgi_param  PATH_INFO          \$fastcgi_path_info;
            fastcgi_param  PATH_TRANSLATED    \$document_root\$fastcgi_path_info;

            fastcgi_param  GATEWAY_INTERFACE  CGI/1.1;
            fastcgi_param  SERVER_SOFTWARE    nginx/\$nginx_version;

            fastcgi_param  REMOTE_ADDR        \$remote_addr;
            fastcgi_param  REMOTE_PORT        \$remote_port;
            fastcgi_param  REMOTE_USER        \$remote_user;
            fastcgi_param  SERVER_ADDR        \$server_addr;
            fastcgi_param  SERVER_NAME        \$server_name;
            fastcgi_param  SERVER_PORT        \$server_port;

            fastcgi_pass_header               Cookie;

            fastcgi_no_cache 1;
        }

        location ~ "^/static/" {
            root $CONF;
        }
    }
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
chmod +x $WEB/test_cgi.cgi

mkdir -p $CONF/circus $CONF/templates $CONF/static
rm -f $LOG/client.log $LOG/server.log

cat > $CONF/circus/cgi.conf <<EOF
{
    "cgi": {
        "templates_path": "$CONF/templates",
        "secure": "No: Test"
    },
    "log": {
        "level": "pii",
        "filename": "$LOG/client.log"
    },
    "variables": {
        "static_path": "/static"
    }
}
EOF

cat > $CONF/circus/server.conf <<EOF
{
    "vault": {
        "filename": "$base.vault"
    },
    "log": {
        "level": "pii",
        "filename": "$LOG/server.log"
    }
}
EOF

(
    export PATH=/bin:/usr/bin
    export HOME=$DIR
    # cannot use valgrind (illegal instruction in libgcrypt)
    exec valgrind --verbose --leak-check=full --track-origins=yes --trace-children=yes --log-file=$LOG/valgrind_server_install.log \
         $ROOT/exe/main/server$exe --install admin password
    # exec $ROOT/exe/main/server$exe --install admin password
) >$LOG/server_install.out 2>$LOG/server_install.err

(
    export PATH=/bin:/usr/bin
    export HOME=$DIR
    # cannot use valgrind (illegal instruction in libgcrypt)
    exec valgrind --verbose --track-origins=yes --trace-children=yes --log-file=$LOG/valgrind_server.log \
          --leak-check=full --show-leak-kinds=all \
         $ROOT/exe/main/server$exe
    # exec $ROOT/exe/main/server$exe
) >$LOG/server.out 2>$LOG/server.err &
server_pid=$!

(
    cd $DIR
    exec /usr/bin/spawn-fcgi -P $RUN/fcgiwrap.pid -F 1 -s $fcgiwrap_socket -- /usr/sbin/fcgiwrap -f
)&
fcgiwrap_pid=$!

(
    cd $DIR
    exec /usr/sbin/nginx -c $CONF/nginx.conf
) &
nginx_pid=$!

sleep 1

ps -p $server_pid,$nginx_pid,$fcgiwrap_pid -F > $base.ps
