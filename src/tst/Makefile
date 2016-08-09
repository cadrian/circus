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

ROOTSRC=$(shell dirname $$(pwd))

# ----------------------------------------------------------------

T_SRC=$(shell find . -name test\*.c)
T_OBJ=$(T_SRC:.c=.o)
T_EXE=$(T_SRC:.c=.exe)
T_RUN=$(T_SRC:.c=.run)
T_DEP=$(T_OBJ:.o=.d)

U_SRC=$(shell find . -name _test\*.c)
U_OBJ=$(U_SRC:.c=.o)
U_DEP=$(U_OBJ:.o=.d)

LDFLAGS_DBG := -Wl,--wrap=mlock -Wl,--wrap=munlock -Wl,--wrap=gcry_randomize -Wl,--wrap=gcry_create_nonce -Wl,--wrap=now

# ----------------------------------------------------------------

.PHONY: test clean

test: $(T_EXE) $(T_RUN)

clean:
	$(shell while read pattern; do find . -name "$$pattern" -exec rm -rf {} +; done < .gitignore)
	rm -rf manual_test
	find . -name \*-conf.d -exec rm -rf {} +
	find . -name \*.[od] -exec rm -f {} +
	find . -name \*.exe -exec rm -f {} +
	find . -name \*.[od] -exec rm -f {} +
	find . -name \*.exe -exec rm -f {} +
	find . -name \*.log.new -exec rm -f {} +
	find . -name \*.log.err -exec rm -f {} +
	find . -name \*.log.valgrind\* -exec rm -f {} +
	find . -name \*.log -exec rm -f {} +
	find . -name \*.db -exec rm -f {} +
	find . -name \*.d0 -exec rm -f {} +
	find . -name \*.d1 -exec rm -f {} +

# ----------------------------------------------------------------

# pull in dependency info for *existing* .o files
-include $(T_OBJ:.o=.d) $(U_OBJ:.o=.d)

%.o: %.c
	$(MAKE) -C ../exe test
	gcc -std=gnu11 -Wall -Wextra -Werror -Wshadow -Wstrict-overflow -fno-strict-aliasing -Wno-missing-field-initializers \
		-fsanitize=undefined -I$(ROOTSRC)/inc -c $(CFLAGS) $*.c -o $*.o
	gcc -MM -fsanitize=undefined -I$(ROOTSRC)/inc -c $(CFLAGS) $*.c > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

%.exe: %.o $(U_OBJ)
	gcc -std=gnu11 -Wall -Wextra -Wshadow -Wstrict-overflow -fno-strict-aliasing -Wno-missing-field-initializers -fsanitize=undefined \
		-o $@ $*.o $(U_OBJ) $(ROOTSRC)/exe/circus.o \
		$(shell $(ROOTSRC)/dep.sh $(ROOTSRC) $*.o $(U_OBJ)) \
		-lcad -lyacjp -luv -lzmq -lsqlite3 -lgcrypt -lcallback \
		$(LDFLAGS_DBG)

%.run: %.exe
	./run_test.sh $*.exe $*.run || { rm -f $*.run; false; }
