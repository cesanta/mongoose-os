CFLAGS ?= -W -Wall -I.. -I.
all: test test_poison test_integrity test_poison_integrity test_poison_integrity_onfree

test:
	$(CC) $(CFLAGS) ../umm_malloc.c umm_malloc_test.c -o test_umm && ./test_umm

test_poison:
	$(CC) $(CFLAGS) -DUMM_POISON -DUMM_DISABLE_VERBOSE_INTEGRITY_CHECK ../umm_malloc.c umm_malloc_test.c -o test_umm && ./test_umm

test_integrity:
	$(CC) $(CFLAGS) -DUMM_POISON -DUMM_INTEGRITY_CHECK -DUMM_DISABLE_VERBOSE_INTEGRITY_CHECK ../umm_malloc.c umm_malloc_test.c -o test_umm && ./test_umm

test_poison_integrity:
	$(CC) $(CFLAGS) -DUMM_POISON -DUMM_INTEGRITY_CHECK -'DUMM_ONFREE(ptr, size)=memset(ptr, 0xff, size)' -DUMM_DISABLE_VERBOSE_INTEGRITY_CHECK  -DUMM_DISABLE_VERBOSE_INTEGRITY_CHECK ../umm_malloc.c umm_malloc_test.c -o test_umm && ./test_umm

test_poison_integrity_onfree:
	$(CC) $(CFLAGS) -DUMM_INTEGRITY_CHECK -DUMM_DISABLE_VERBOSE_INTEGRITY_CHECK ../umm_malloc.c umm_malloc_test.c -o test_umm && ./test_umm

