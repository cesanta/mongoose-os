# Copyright (c) 2014 Cesanta Software
# All rights reserved
#
# Makefile for the Smart.js engine

NS = ../net_skeleton
V7 = ../v7

SOURCES = engine.c $(NS)/net_skeleton.c $(V7)/v7.c
BINARY = engine

CFLAGS = -I$(NS) -I$(V7) $(CFLAGS_EXTRA) -W -Wall

all: $(BINARY)

$(BINARY): $(SOURCES)
	cd $(V7) && make v7.c
	$(CC) $(SOURCES) -o $@ -W -Wall -g $(CFLAGS)

w:
	wine cl /MD /TC /nologo /DNDEBUG /O1 $(CFLAGS) $(SOURCES)

$(NS)/net_skeleton.c:
	-@cd .. && git clone https://github.com/cesanta/net_skeleton

run: $(BINARY)
	$(BINARY) example.js

$(V7)/v7.c: $(V7)/v7.h
	test -d $(V7) || ( cd .. && git clone https://github.com/cesanta/v7 )

clean:
	rm -rf $(BINARY) *.exe *.obj *.o *.dSYM
