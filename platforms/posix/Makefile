V7_PATH = ../../../v7
SRC_PATH = ../../src
COMMON_PATH = ../../../common
BINDIR = ./bin
OUTAPP = smartjs

V7_FEATURES = -DV7_BUILD_PROFILE=3
CC = cc

INCLUDE = -I$(V7_PATH) -I$(SRC_PATH) -I$(COMMON_PATH)
VERSION_FILE = sj_version_obj.c
SOURCES = $(wildcard *.c) $(V7_PATH)/v7.c $(SRC_PATH)/sj_v7_ext.c $(SRC_PATH)/sj_conf.c

CFLAGS = -O2 -Wall
COMPILE = $(CC) $(CFLAGS) $(INCLUDE)
LIBS = -lm -lrt

.PHONE: all clean

all: $(BINDIR)/$(OUTAPP)

$(BINDIR)/$(OUTAPP): $(SOURCES)
	@echo "GEN "$(VERSION_FILE)
	@REF=$$(git symbolic-ref HEAD | sed 's:refs/heads/::'); \
	SHA=$$(git rev-parse HEAD | head -c 8); \
	DATE=$$(TZ=GMT date +"%Y%m%d-%H%M%S"); \
 	echo "const char *sj_version = \"POSIX-$$REF/$$DATE/$$SHA\";" > $(VERSION_FILE)
	@mkdir -p $(BINDIR)
	@echo Compiling
	@$(CC) -O2 $(INCLUDE) $(SOURCES) $(VERSION_FILE) $(LIBS) -o $(BINDIR)/$(OUTAPP)
	
clean:
	@rm -f $(BINDIR)/$(OUTAPP) $(VERSION_FILE)
