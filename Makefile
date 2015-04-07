# Copyright (c) 2014-2015 Cesanta Software
# All rights reserved
#
# Makefile for the Smart.js engine

CFLAGS = -DNS_ENABLE_SSL -DV7_MAIN -DV7_DISABLE_SOCKETS -Isrc/dependencies $(CFLAGS_EXTRA)
SOURCES = src/main.c \
					src/api_socket.c \
					src/api_crypto.c \
					src/api_file.c \
					src/api_os.c \
					src/dependencies/fossa.c \
					src/dependencies/v7.c \
					src/dependencies/krypton.c
BINARY = sj

# TODO(lsm): reuse established test infrastructure as we move to a dev repo
all: $(BINARY) unit_test
	./unit_test

unit_test: test/unit_test.c
	$(CC) -o $@ test/unit_test.c $(SOURCES) -g $(CFLAGS) -DUNIT_TEST

$(BINARY): $(SOURCES)
	$(CC) $(SOURCES) -o $@ -W -Wall -g -lm $(CFLAGS)

sync:
	cp ../fossa/fossa.[ch] ../v7/v7.[ch] ../krypton/krypton.c src/dependencies/

w:
	wine cl /MD /TC /nologo /DNDEBUG /O1 $(CFLAGS) $(SOURCES)

clean:
	rm -rf $(BINARY) *.exe *.obj *.o *.dSYM unit_test
