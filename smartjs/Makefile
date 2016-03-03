CLANG_FORMAT:=clang-format

ifneq ("$(wildcard /usr/local/bin/clang-3.6)","")
	CLANG:=/usr/local/bin/clang-3.6
	CLANG_FORMAT:=/usr/local/bin/clang-format-3.6
endif

all: format
	$(MAKE) -C platforms/posix

format:
	@(for i in src platforms/cc3200 platforms/esp8266/user platforms/esp8266/tools; do \
       /usr/bin/find $$i -type f -name "*.[ch]" | grep -v /build/; \
    done) | xargs $(CLANG_FORMAT) -i

# allows CI tests and precommit hooks to ensure
# that makefile targets that generate or modify code
# (such as make format) don't introduce unwanted diffs.
difftest:
	@TMP=`mktemp -t checkout-diff.XXXXXX`; \
	git diff  >$$TMP ; \
	if [ -s "$$TMP" ]; then echo found diffs in checkout:; git status -s; head -n 50 "$$TMP"; exit 1; fi; \
	rm $$TMP
