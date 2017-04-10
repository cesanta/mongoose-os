SOURCES = str_util.c cs_time.c unit_test.c test_util.c
CFLAGS = -I.. -g $(CFLAGS_EXTRA)
UMM_MALLOC_TEST_PATH = umm_malloc/test

.PHONY: unit_test

all: unit_test

unit_test:
	make -C $(UMM_MALLOC_TEST_PATH)
	make -C segstack
	cc $(SOURCES) -o $@ $(CFLAGS)
	./$@

clean:
	rm -f *.o unit_test

REPO_ROOT = ..
FORMAT_FILES = *.[ch] platforms/*/*.[ch] mg_rpc/*.[ch] segstack/*.[ch]
include $(REPO_ROOT)/common.mk