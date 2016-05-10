# TODO(dfrank) use tools/md_doc.mk. The problem is that we additionally need
# to generate `cloud/defs/%.service` files

.PHONY: all clean

REPO_ROOT = $(CURDIR)/../..

ALL_GEN_DOCS = services

CLOUD_DEFS_PATH = $(REPO_ROOT)/cloud/defs
CLOUD_EXAMPLES_PATH = $(REPO_ROOT)/cloud/doc/service/examples

DOCGEN_OPTS = --type=clubby_yaml --clubby-examples=$(CLOUD_EXAMPLES_PATH)

SRC_FILES = $(sort $(wildcard $(CLOUD_DEFS_PATH)/*.service.yaml))

all: $(ALL_GEN_DOCS)

clean:
	@rm -rf $(ALL_GEN_DOCS)

services: $(SRC_FILES)

include ../../tools/md_doc.mk

