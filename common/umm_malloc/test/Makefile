
all: test test_poison test_integrity test_poison_integrity

INCDIRS = -I.. -I.

test:
	@echo NORMAL
	gcc --std=c99 $(CFLAGS) $(INCDIRS) -g3 -m32 \
	  ../umm_malloc.c umm_malloc_test.c \
		-o test_umm
	./test_umm

test_poison:
	@echo POISON
	gcc --std=c99 $(CFLAGS) $(INCDIRS) -DUMM_POISON -g3 -m32 \
	  ../umm_malloc.c umm_malloc_test.c \
		-o test_umm
	./test_umm

test_integrity:
	@echo INTEGRITY
	gcc --std=c99 $(CFLAGS) $(INCDIRS) -DUMM_INTEGRITY_CHECK -g3 -m32 \
	  ../umm_malloc.c umm_malloc_test.c \
		-o test_umm
	./test_umm

test_poison_integrity:
	@echo POISON + INTEGRITY
	gcc --std=c99 $(CFLAGS) $(INCDIRS) -DUMM_POISON -DUMM_INTEGRITY_CHECK -g3 -m32 \
	  ../umm_malloc.c umm_malloc_test.c \
		-o test_umm
	./test_umm

