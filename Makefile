# Copyright (c) 2014 Cesanta Software
# All rights reserved

NS = ../net_skeleton
V7 = ../v7
SOURCES = engine.c $(NS)/net_skeleton.c $(V7)/v7.c
BINARY = engine
CFLAGS = -W -Wall -g -I$(NS) -I$(V7) $(CFLAGS_EXTRA)

all: $(BINARY)

$(BINARY): $(SOURCES)
	$(CC) $(SOURCES) -o $@ $(CFLAGS)

$(NS)/net_skeleton.c:
	cd .. && git clone https://github.com/cesanta/net_skeleton

$(V7)/v7.c:
	cd .. && git clone https://github.com/cesanta/v7