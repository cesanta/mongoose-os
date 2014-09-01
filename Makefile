# Copyright (c) 2014 Cesanta Software
# All rights reserved
#
# Makefile for the Smart.js engine

SOURCES = engine/smart.c engine/net_skeleton.c engine/v7.c
BINARY = smart

all: $(BINARY)

$(BINARY): $(SOURCES)
	$(CC) $(SOURCES) -o $@ -W -Wall -g $(CFLAGS_EXTRA)

run: $(BINARY) sync
	./$(BINARY) examples/example.js

sync:
	cd ../v7 && make v7.c
	cp ../net_skeleton/net_skeleton.[ch] ../v7/v7.[ch] engine/

w:
	wine cl /MD /TC /nologo /DNDEBUG /O1 $(CFLAGS_EXTRA) $(SOURCES)

clean:
	rm -rf $(BINARY) *.exe *.obj *.o *.dSYM
