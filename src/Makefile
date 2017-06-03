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
# Copyright © 2015-2016 Cyril Adrian <cyril.adrian@gmail.com>

# ----------------------------------------------------------------

# With the help of http://scottmcpeak.com/autodepend/autodepend.html

ROOTSRC=$(shell pwd)

# ----------------------------------------------------------------

.PHONY: all test compile manual_test clean test_nginx test_lighttpd

all: test

test: compile
	$(MAKE) -C tst $@

manual_test: test
	$(MAKE) -C exe $@

test_nginx: manual_test
	$(MAKE) -C tst $@

test_lighttpd: manual_test
	$(MAKE) -C tst $@

compile:
	$(MAKE) -C exe $@
	$(MAKE) -C web $@

clean:
	$(MAKE) -C exe $@
	$(MAKE) -C tst $@
