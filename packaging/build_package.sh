#!/usr/bin/env bash

set -e

ROOTDIR=$(dirname $(dirname $(readlink -f $0)))

function build() {
    PACKAGING=$ROOTDIR/packaging

    local date=$(date -R)
    local snap=$(date -u +'~%Y%m%d%H%M%S')
    echo Date is "$date"
    if head -n 1 $PACKAGING/debian.skel/changelog | fgrep -q '#SNAPSHOT#'; then
        echo Version is SNAPSHOT: "$snap"
    else
        echo Version is $(head -n 1 $tgt/changelog | awk -F'[()]' '{print $$2}')
    fi

    for package in "$@"; do
        cd $ROOTDIR

        export TARGET=$PACKAGING/target/$package/build
        tgt=$TARGET/debian
        rm -rf $TARGET
        mkdir -p $tgt

        echo Copying skeleton
        cp -a $ROOTDIR/packaging/debian.skel/* $tgt/
        echo Copying layer: $package
        cp -a $ROOTDIR/packaging/debian.$package/* $tgt/

        {
            cat $PACKAGING/debian.skel/control
            echo
            cat $PACKAGING/debian.$package/control
        } > $tgt/control

        echo Customizing
        sed "s/#DATE#/${date}/;s/#SNAPSHOT#/${snap}/" -i $tgt/changelog
        version=$(head -n 1 $tgt/changelog | awk -F'[()]' '{print $2}')
        echo Version is "$version"

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
\t\$(MAKE) -C $ROOTDIR \$@

clean:
\t\$(MAKE) -C $ROOTDIR \$@

install: compile
\t#env | sort
\tmkdir -p \$(DESTDIR)/usr/lib/circus
\tcp -a $ROOTDIR/src/exe/main/$package.exe \$(DESTDIR)/usr/lib/circus/
EOF

        echo Building $package
        cd $TARGET
        debuild -us -uc -nc
    done
}

case x"$1" in
    xserver|xclient_cgi)
        build $1
        ;;
    x)
        build server client_cgi
        ;;
    *)
        echo "Usage: $0 {server|client_cgi}" >&2
        exit 1
        ;;
esac
