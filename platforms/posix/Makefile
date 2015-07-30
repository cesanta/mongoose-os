V7_PATH = ../../../v7
SRC_PATH = ../../src
BINDIR = ./bin
OUTAPP = smartjs

V7_FEATURES = -DV7_BUILD_PROFILE=3
CC = cc

INCLUDE = -I$(V7_PATH) -I$(SRC_PATH)
SOURCES = $(wildcard *.c) $(V7_PATH)/v7.c $(SRC_PATH)/sj_v7_ext.c
OBJS = $(SRCS:.c=.o) v7.o

CFLAGS = -O2 -Wall
COMPILE = $(CC) $(CFLAGS) $(INCLUDE)
LIBS = -lm -lrt

.PHONE: all clean

all: $(BINDIR)/$(OUTAPP)

$(BINDIR)/$(OUTAPP): $(SOURCES)
	@mkdir -p $(BINDIR)
	@$(CC) -O2 $(INCLUDE) $(SOURCES) $(LIBS) -o $(BINDIR)/$(OUTAPP)
	
clean:
	@rm -f $(BINDIR)/$(OUTAPP)
