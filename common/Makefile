SOURCES = str_util.c unit_test.c test_util.c
CFLAGS = -g $(CFLAGS_EXTRA)

CLANG_FORMAT:=clang-format

ifneq ("$(wildcard /usr/local/bin/clang-3.6)","")
	CLANG_FORMAT:=/usr/local/bin/clang-format-3.6
endif

.PHONY: unit_test

all: unit_test

unit_test: $(SOURCES)
	cc $(SOURCES) -o $@ $(CFLAGS)
	./$@

clean:
	rm -f *.o unit_test

format:
	@$(CLANG_FORMAT) -i *.[ch] platforms/esp8266/*.[ch]

