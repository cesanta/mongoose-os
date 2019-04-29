SOURCES = str_util.c cs_dbg.c cs_time.c unit_test.c test_main.c test_util.c cs_varint.c mg_str.c
CFLAGS = -I.. -g $(CFLAGS_EXTRA)
UMM_MALLOC_TEST_PATH = umm_malloc/test

.PHONY: unit_test

all: unit_test


unit_test:
	$(CC) -Wall -Werror $(SOURCES) -o $@ $(CFLAGS)
	./$@

test: unit_test
	make -C $(UMM_MALLOC_TEST_PATH) 
	make -C segstack

clean:
	rm -f *.o unit_test *.obj _CL_*

ci-test: vc2017 unit_test

CLFLAGS = /DWIN32_LEAN_AND_MEAN /MD /O2 /TC /W2 /WX /I..
vc98 vc2017:
	docker run -v $$(pwd):$$(pwd) -w $$(pwd) docker.cesanta.com/$@ wine cl $(SOURCES) $(CLFLAGS) /Fe$@.exe
	docker run -v $$(pwd):$$(pwd) -w $$(pwd) docker.cesanta.com/$@ wine $@.exe 

REPO_ROOT = ..
FORMAT_FILES = *.[ch] platforms/*/*.[ch] mg_rpc/*.[ch] segstack/*.[ch]
include $(REPO_ROOT)/common.mk
