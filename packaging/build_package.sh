#!/usr/bin/env bash

set -e

ROOTDIR=$(dirname $(dirname $(readlink -f $0)))
PACKAGING=$ROOTDIR/packaging

date=$(date -R)
snap=$(date -u +'~%Y%m%d%H%M%S')
echo Date is "$date"
if head -n 1 $PACKAGING/debian.skel/changelog | fgrep -q '#SNAPSHOT#'; then
    echo Changelog version is SNAPSHOT: "$snap"
else
    echo Changelog version is $(head -n 1 $tgt/changelog | awk -F'[()]' '{print $$2}')
fi

export TARGET=$PACKAGING/target/build
tgt=$TARGET/debian
rm -rf $TARGET
mkdir -p $tgt

echo Copying skeleton
cp -a $ROOTDIR/packaging/debian.skel/* $tgt/

echo Copying source
cp -a $ROOTDIR/{src,static,templates,packaging/conf} $TARGET/

echo Computing version
sed "s/#DATE#/${date}/;s/#SNAPSHOT#/${snap}/" -i $tgt/changelog
version=$(head -n 1 $tgt/changelog | awk -F'[()]' '{print $2}')
echo Package version is "$version"

echo Computing control dependencies
arch=$(dpkg-architecture -q DEB_BUILD_ARCH)
echo Architecture: $arch
egrep -o '%[^%]+%' $tgt/control | sed 's/%//g' | fmt -1 | while IFS=: read section dep; do
    pkg=$(dpkg-query -f '${Section}:${Package}:${Version}:${Architecture}\n' -W "$dep*" |
                 awk -F: '$1 == "'$section'" && $4 == "'$arch'" {printf("%s (>= %s)\n", $2, $3)}')
    if [ -z "$pkg" ]; then
        echo "$dep not found!" >&2
        dpkg-query -f '${Section}:${Package}:${Version}:${Architecture}\n' -W "$dep" >&2
        exit 1
    fi
    echo "Dependency: $dep" '=>' "$pkg"
    sed -r "s/%[^:]+:$(echo "$dep" | sed 's/\*/\\*/g')%/$pkg/" -i $tgt/control
done
sed 's/#VERSION#/'"$version"'/g' -i $tgt/control

sed 's/\\t/\t/g' > $TARGET/Makefile <<EOF
export TARGET=$TARGET

.PHONY: all compile clean install

all: compile

compile:
\t\$(MAKE) -C src \$@

clean:
\t\$(MAKE) -C src \$@

install: compile
\t#env | sort
\tmkdir -p \$(DESTDIR)/usr/lib/circus \$(DESTDIR)/etc/circus
\tcp -a src/exe/main/server.exe \$(DESTDIR)/usr/lib/circus/
\tcp -a src/exe/main/client_cgi.exe \$(DESTDIR)/usr/lib/circus/
\tcp -a static \$(DESTDIR)/etc/circus/
\tcp -a templates \$(DESTDIR)/etc/circus/
\tcp -a conf/* \$(DESTDIR)/etc/circus/
EOF

echo Building
cd $TARGET
debuild -us -uc -nc -F
