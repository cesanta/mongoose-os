# Copyright (c) 2014 Cesanta Software
# All rights reserved

NS = ../net_skeleton
V7 = ../v7
SLRE = ../slre

SOURCES = engine.c $(NS)/net_skeleton.c $(V7)/v7.c $(SLRE)/slre.c
BINARY = engine

CFLAGS = -I$(NS) -I$(V7) -I$(SLRE) $(CFLAGS_EXTRA)

all: $(BINARY)

$(BINARY): $(SOURCES)
	$(CC) $(SOURCES) -o $@ -W -Wall -g $(CFLAGS)

w:
	wine cl /MD /TC /nologo /DNDEBUG /O1 $(CFLAGS) $(SOURCES)

$(NS)/net_skeleton.c:
	cd .. && git clone https://github.com/cesanta/net_skeleton

$(V7)/v7.c:
	cd .. && git clone https://github.com/cesanta/v7

$(SLRE)/slre.c:
	cd .. && git clone https://github.com/cesanta/slre

clean:
	rm -rf $(BINARY) *.exe *.obj *.o