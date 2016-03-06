#!/usr/bin/env bash

#    This file is part of Circus.
#
#    Circus is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License.
#
#    Circus is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with Circus.  If not, see <http://www.gnu.org/licenses/>.
#
#    Copyright © 2015-2016 Cyril Adrian <cyril.adrian@gmail.com>

base=$(dirname $(readlink -f $0))/$(basename $0 .sh)
rm -f $base.client_log

. $(dirname $(readlink -f $0))/_test_client_cgi_setup.sh

mkdir -p $RUN/circus $CONF/templates
cat > $RUN/circus/cgi.conf <<EOF
{
    "cgi": {
        "templates_path": "$CONF/templates"
    },
    "log": {
        "level": "debug",
        "filename": "$base.client_log"
    }
}
EOF

cat >$CONF/templates/login.tpl <<EOF
<html>
<head>
<title>LOGIN</title>
</head>
<body>
<p>
Please enter your credentials
</p>
<form>
<input type="text" name="username"/>
<input type="password" name="password"/>
</form>
</body>
</html>
EOF

RUN=$base.run
curl -m10 'http://test:pwd@localhost:8888/test_cgi.cgi' -o $base.res

. $(dirname $(readlink -f $0))/_test_client_cgi_teardown.sh

cat $base.res
