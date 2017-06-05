#!/usr/bin/env make

# This file is part of Circus.
#
# Circus is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License.
#
# Circus is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Circus.  If not, see <http://www.gnu.org/licenses/>.
#
# Copyright © 2015-2017 Cyril Adrian <cyril.adrian@gmail.com>

# ----------------------------------------------------------------

.PHONY: all test manual_test compile clean debian test_nginx test_lighttpd

all: test doc
	$(MAKE) -C src $@

doc: README.html

README.html: README.md
	markdown_py -o html5 -e UTF-8 -x tables -f README.html < README.md

test: compile
	$(MAKE) -C src $@

manual_test: test
	$(MAKE) -C src $@

test_nginx: manual_test
	$(MAKE) -C src $@

test_lighttpd: manual_test
	$(MAKE) -C src $@

compile:
	$(MAKE) -C src $@

clean:
	$(MAKE) -C src $@
	rm -rf packaging/target

debian:
	packaging/build_package.sh
