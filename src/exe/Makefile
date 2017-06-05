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

ROOTSRC=$(shell dirname $$(pwd))

# ----------------------------------------------------------------

SRC=$(shell find . -name gen -prune -o -name \*.c -print)
S_SRC=$(shell find . \( -name gen -o -name main -o -name client \) -prune -o -name \*.c -print)
C_SRC=$(shell find . \( -name gen -o -name main -o -name server \) -prune -o -name \*.c -print)

OBJ=$(SRC:.c=.o)
S_OBJ=$(S_SRC:.c=.o)
C_OBJ=$(C_SRC:.c=.o)

DEP=$(OBJ:.o=.d)

LDFLAGS ?=

# mlock/munlock: we are not root
# gcry_*: we need valgrind (libgcrypt generates instructions not recognized by Valgrind)
# now/init: reproducible test logs
LDFLAGS_DBG := -Wl,--wrap=mlock -Wl,--wrap=munlock -Wl,--wrap=gcry_randomize -Wl,--wrap=gcry_create_nonce -Wl,--wrap=now -Wl,--wrap=init

# mlock/munlock: we are not root
# gcry_*: we need valgrind (libgcrypt generates instructions not recognized by Valgrind)
LDFLAGS_RND := -Wl,--wrap=mlock -Wl,--wrap=munlock -Wl,--wrap=gcry_randomize_rnd -Wl,--wrap=gcry_create_nonce_rnd

CFLAGS ?= -g -O0

# ----------------------------------------------------------------

.PHONY: compile test manual_test clean

compile: main/server.exe main/client_cgi.exe

test: main/server.dbg.exe main/client_cgi.dbg.exe

manual_test: main/server.rnd.exe main/client_cgi.rnd.exe

clean:
	find . -name \*.[od] -exec rm -f {} +
	find . -name \*.exe -exec rm -f {} +
	rm -rf protocol/{gen,messages}
	rm -rf client/{gen,web}

# ----------------------------------------------------------------

# pull in dependency info for *existing* .o files
-include $(OBJ:.o=.d)

%.o: %.c protocol/messages client/web
	gcc -std=gnu11 -Wall -Wextra -Werror -Wshadow -Wstrict-overflow -fno-strict-aliasing -Wno-missing-field-initializers \
		-I$(ROOTSRC)/inc -c $(CFLAGS) $*.c -o $*.o
	gcc -MM -I$(ROOTSRC)/inc -c $(CFLAGS) $*.c > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

protocol/messages: protocol/messages.sh protocol/messages.json
	(cd protocol && bash messages.sh) > $@

client/web: client/web.sh client/web.json protocol/messages.json
	(cd client && bash web.sh) > $@

main/server.exe: protocol/messages main/server.o $(S_OBJ)
	gcc -std=gnu11 -Wall -Wextra -Wshadow -Wstrict-overflow -fno-strict-aliasing -Wno-missing-field-initializers \
		$(CFLAGS) $(LDFLAGS) -Wl,--gc-sections -o $@ main/server.o $(S_OBJ) \
		-lcad -lyacjp -luv -lzmq -lsqlite3 -lgcrypt

main/client_cgi.exe: protocol/messages client/web main/client_cgi.o $(C_OBJ)
	gcc -std=gnu11 -Wall -Wextra -Wshadow -Wstrict-overflow -fno-strict-aliasing -Wno-missing-field-initializers \
		$(CFLAGS) $(LDFLAGS) -Wl,--gc-sections -o $@ main/client_cgi.o $(C_OBJ) \
		-lcad -lyacjp -luv -lzmq

main/server.dbg.exe: main/server.exe
	gcc -std=gnu11 -Wall -Wextra -Wshadow -Wstrict-overflow -fno-strict-aliasing -Wno-missing-field-initializers \
		-fsanitize=undefined \
		$(CFLAGS) $(LDFLAGS_DBG) $(LDFLAGS) -Wl,--gc-sections -o $@ main/server.o $(S_OBJ) \
		-lcad -lyacjp -luv -lzmq -lsqlite3 -lgcrypt

main/client_cgi.dbg.exe: main/client_cgi.exe
	gcc -std=gnu11 -Wall -Wextra -Wshadow -Wstrict-overflow -fno-strict-aliasing -Wno-missing-field-initializers \
		-fsanitize=undefined \
		$(CFLAGS) $(LDFLAGS_DBG) $(LDFLAGS) -Wl,--gc-sections -o $@ main/client_cgi.o $(C_OBJ) \
		-lcad -lyacjp -luv -lzmq

main/server.rnd.exe: main/server.exe
	gcc -std=gnu11 -Wall -Wextra -Wshadow -Wstrict-overflow -fno-strict-aliasing -Wno-missing-field-initializers \
		-fsanitize=undefined \
		$(CFLAGS) $(LDFLAGS_RND) $(LDFLAGS) -Wl,--gc-sections -o $@ main/server.o $(S_OBJ) \
		-lcad -lyacjp -luv -lzmq -lsqlite3 -lgcrypt

main/client_cgi.rnd.exe: main/client_cgi.exe
	gcc -std=gnu11 -Wall -Wextra -Wshadow -Wstrict-overflow -fno-strict-aliasing -Wno-missing-field-initializers \
		-fsanitize=undefined \
		$(CFLAGS) $(LDFLAGS_RND) $(LDFLAGS) -Wl,--gc-sections -o $@ main/client_cgi.o $(C_OBJ) \
		-lcad -lyacjp -luv -lzmq
