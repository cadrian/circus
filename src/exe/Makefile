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

SRC=$(shell find . -name gen -prune -o -name \*.c -print)
S_SRC=$(shell find . \( -name gen -o -name main -o -name client \) -prune -o -name \*.c -print)
C_SRC=$(shell find . \( -name gen -o -name main -o -name server \) -prune -o -name \*.c -print)

OBJ=$(SRC:.c=.o)
S_OBJ=$(S_SRC:.c=.o)
C_OBJ=$(C_SRC:.c=.o)

DEP=$(OBJ:.o=.d)

LD_FLAGS_DBG ?= -Wl,--wrap=mlock -Wl,--wrap=munlock -Wl,--wrap=gcry_randomize -Wl,--wrap=gcry_create_nonce -Wl,--wrap=now

# ----------------------------------------------------------------

.PHONY: clean compile

compile: main/server.exe main/client.exe

test: main/server.dbg.exe main/client.dbg.exe

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
		-fsanitize=undefined -I$(ROOTSRC)/inc -c $(CFLAGS) $*.c -o $*.o
	gcc -MM -fsanitize=undefined -I$(ROOTSRC)/inc -c $(CFLAGS) $*.c > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

protocol/messages: protocol/messages.sh protocol/messages.json
	(cd protocol && messages.sh) > $@

client/web: client/web.sh client/web.json protocol/messages.json
	(cd client && web.sh) > $@

main/server.exe: protocol/messages main/server.o $(S_OBJ)
	gcc -std=gnu11 -Wall -Wextra -Wshadow -Wstrict-overflow -fno-strict-aliasing -Wno-missing-field-initializers -fsanitize=undefined \
		-lcad -lyacjp -luv -lzmq -lsqlite3 -lgcrypt \
		$(LD_FLAGS) -o $@ main/server.o $(S_OBJ)

main/client.exe: protocol/messages client/web main/client_cgi.o $(C_OBJ)
	gcc -std=gnu11 -Wall -Wextra -Wshadow -Wstrict-overflow -fno-strict-aliasing -Wno-missing-field-initializers -fsanitize=undefined \
		-lcad -lyacjp -luv -lzmq \
		$(LD_FLAGS) -o $@ main/client_cgi.o $(C_OBJ)

main/server.dbg.exe: main/server.exe
	gcc -std=gnu11 -Wall -Wextra -Wshadow -Wstrict-overflow -fno-strict-aliasing -Wno-missing-field-initializers -fsanitize=undefined \
		-lcad -lyacjp -luv -lzmq -lsqlite3 -lgcrypt \
		$(LD_FLAGS_DBG) $(LD_FLAGS) -o $@ main/server.o $(S_OBJ)

main/client.dbg.exe: main/client.exe
	gcc -std=gnu11 -Wall -Wextra -Wshadow -Wstrict-overflow -fno-strict-aliasing -Wno-missing-field-initializers -fsanitize=undefined \
		-lcad -lyacjp -luv -lzmq \
		$(LD_FLAGS_DBG) $(LD_FLAGS) -o $@ main/client_cgi.o $(C_OBJ)
