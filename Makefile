# Copyright (c) 2014 Cesanta Software
# All rights reserved
#
# Makefile for the Smart.js engine

CFLAGS = -DNS_ENABLE_SSL $(CFLAGS_EXTRA)
SOURCES = engine/smart.c engine/net_skeleton.c engine/v7.c
BINARY = smartjs

all: $(BINARY)

$(BINARY): $(SOURCES)
	$(CC) $(SOURCES) -o $@ -W -Wall -g -lssl -lm $(CFLAGS)

run: $(BINARY)
	cd ../cesanta.com && make
#	./$(BINARY) examples/example.js

sync:
	cd ../v7 && make v7.c
	cp ../net_skeleton/net_skeleton.[ch] ../v7/v7.[ch] engine/

w:
	wine cl /MD /TC /nologo /DNDEBUG /O1 $(CFLAGS) $(SOURCES)

clean:
	rm -rf $(BINARY) *.exe *.obj *.o *.dSYM
