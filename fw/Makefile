all: format

# allows CI tests and precommit hooks to ensure
# that makefile targets that generate or modify code
# (such as make format) don't introduce unwanted diffs.
difftest:
	@TMP=`mktemp -t checkout-diff.XXXXXX`; \
	git diff  >$$TMP ; \
	if [ -s "$$TMP" ]; then echo found diffs in checkout:; git status -s; head -n 50 "$$TMP"; exit 1; fi; \
	rm $$TMP

REPO_ROOT = ..
FORMAT_FILES = 'examples/*/*/*.[ch]' 'examples/*/*/*.cpp' \
               'platforms/cc3200/src/*.[ch]' \
               'platforms/esp32/src/*.[ch]' \
               'platforms/esp8266/*.[ch]' \
               'src/*.[ch]' 'src/*.cpp'
include $(REPO_ROOT)/common.mk
