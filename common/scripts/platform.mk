OS?=
CLANG:=clang
CLANG_TIDY:=clang-tidy
LLVM_SYMBOLIZER:=/usr/bin/llvm-symbolizer

# might be useful to use lldb
ifneq ("$(wildcard /usr/local/bin/clang-3.5)","")
	CLANG:=/usr/local/bin/clang-3.5
endif

#### TODO(mkm): split

# disable optimizations and sockets on windows
DEFS_WINDOWS=-DV7_DISABLE_SOCKETS
CFLAGS_WINDOWS:=-O0 $(DEFS_WINDOWS)
CFLAGS_PLATFORM:=

# not all environments set the same env vars
# so we have to try a few
ifdef SYSTEMROOT
	CFLAGS_PLATFORM:=$(CFLAGS_WINDOWS)
endif
ifeq ($(OS),Windows_NT)
	CFLAGS_PLATFORM:=$(CFLAGS_WINDOWS)
endif

IS_GCC:=$(shell echo "\#if defined(__GNUC__) && !defined(__clang__)\n\#error IS_GCC\n\#endif" | $(CC) -E - 2>&1 |grep -q IS_GCC && echo 1)

TMP_PERL:=$(shell perl -e 'print $$^X' 2>/dev/null)
ifneq ("${TMP_PERL}","")
	PERL:="${TMP_PERL}"
endif
