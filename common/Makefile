SOURCES = str_util.c cs_time.c unit_test.c test_util.c
CFLAGS = -I.. -g $(CFLAGS_EXTRA)
UMM_MALLOC_TEST_PATH = umm_malloc/test

CLANG_FORMAT:=clang-format

ifneq ("$(wildcard /usr/local/bin/clang-3.6)","")
	CLANG_FORMAT:=/usr/local/bin/clang-format-3.6
endif

.PHONY: unit_test

all: unit_test

unit_test:
	make -C $(UMM_MALLOC_TEST_PATH)
	cc $(SOURCES) -o $@ $(CFLAGS)
	./$@

clean:
	rm -f *.o unit_test

format:
	@$(CLANG_FORMAT) -i *.[ch] platforms/*/*.[ch] mg_rpc/*.[ch]

