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

.PHONY: all test manual_test compile clean

all: test
	$(MAKE) -C src $@

test: compile
	$(MAKE) -C src $@

manual_test: test
	$(MAKE) -C src $@

compile:
	$(MAKE) -C src $@

clean:
	$(MAKE) -C src $@
