# Copyright (c) 2014 Cesanta Software
# All rights reserved

NS = ../net_skeleton
V7 = ../v7
SOURCES = engine.c $(NS)/net_skeleton.c $(V7)/v7.c
BINARY = engine

CFLAGS = -I$(NS) -I$(V7) $(CFLAGS_EXTRA)

all: $(BINARY)

$(BINARY): $(SOURCES)
	$(CC) $(SOURCES) -o $@ -W -Wall -Weverything -g $(CFLAGS)

w:
	wine cl /MD /TC /nologo /DNDEBUG /O1 $(CFLAGS) $(SOURCES)

$(NS)/net_skeleton.c:
	cd .. && git clone https://github.com/cesanta/net_skeleton

$(V7)/v7.c:
	cd .. && git clone https://github.com/cesanta/v7

clean:
	rm -rf $(BINARY) $(BINARY).exe