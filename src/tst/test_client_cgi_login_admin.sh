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

. $(dirname $(readlink -f $0))/_test_client_cgi_setup.sh

cat >$CONF/templates/login.tpl <<EOF
<html>
    <head>
        <title>LOGIN</title>
    </head>
    <body>
        <p>
            Please enter your credentials
        </p>
        <form action="login.do">
            <input type="text" name="userid"/>
            <input type="password" name="password"/>
            <input type="submit" name="action" value="ok"/>
        </form>
    </body>
</html>
EOF

cat > $CONF/templates/admin_home.tpl <<EOF
<html>
    <head>
        <title>Home</title>
    </head>
    <body>
        Home.
        <input type="hidden" value="{{token}}"/>
    </body>
</html>
EOF

RUN=$base.run
curl -c $base.cookies -m10 'http://test:pwd@localhost:8888/test_cgi.cgi' -D $base.01.hdr -o $base.01.res
curl -c $base.cookies -m10 'http://test:pwd@localhost:8888/test_cgi.cgi/login.do' -d userid=admin -d password=password -d action=ok -D $base.02.hdr -o $base.02.res

. $(dirname $(readlink -f $0))/_test_client_cgi_teardown.sh
