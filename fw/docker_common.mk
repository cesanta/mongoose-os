# Common rules for "outer" Makefiles, i.e. the ones that invoke Docker.
#
# App must define its name (APP) and path to MIOT repo (MIOT_PATH).
#
# App's directory and MIOT must intersect at REPO_PATH, which will be determined
# automatically if it's a Git repo, otherwise REPO_PATH must be set explicitly.

MAKEFLAGS += --warn-undefined-variables
REPO_PATH ?= $(shell git rev-parse --show-toplevel)
APP_VERSION ?=
APP_BUILD_ID ?=
PYTHON ?= python
DOCKER_EXTRA ?=

REPO_ABS_PATH = $(realpath $(REPO_PATH))
MIOT_ABS_PATH = $(realpath $(MIOT_PATH))
MIOT_REPO_SUBDIR = $(subst $(REPO_ABS_PATH),,$(MIOT_ABS_PATH))
APP_ABS_PATH = $(CURDIR)
APP_SUBDIR ?= $(subst $(REPO_ABS_PATH),,$(APP_ABS_PATH))

MIOT_PLATFORM_SUBDIR = $(MIOT_REPO_SUBDIR)/fw/platforms/$(APP_PLATFORM)
MIOT_PLATFORM_ABS_PATH = $(REPO_ABS_PATH)/$(MIOT_PLATFORM_SUBDIR)
SDK_VERSION ?= $(shell cat $(MIOT_PLATFORM_ABS_PATH)/sdk.version)

# this hack is needed to make `-$(MAKEFLAGS)` always work (notice the dash).
# Otherwise, $(MAKEFLAGS) does not contain the flag `w` when `make` runs
# directly, but it does contain this flag when it runs as a submake.
#
# see:
# - http://www.gnu.org/software/make/manual/html_node/Options_002fRecursion.html
# - http://www.gnu.org/software/make/manual/html_node/_002dw-Option.html
MAKEFLAGS += w

T=$(shell [ -t 0 ] && echo true || echo false)

all clean:
	@docker run --rm -i --tty=$T \
	  -v $(REPO_ABS_PATH):/src \
	  $(DOCKER_EXTRA) $(SDK_VERSION) \
	  /bin/bash -c "\
	    nice make -j4 \
	      -C /src$(APP_SUBDIR) -f Makefile.build \
	      APP=$(APP) \
	      APP_VERSION=$(APP_VERSION) \
	      APP_BUILD_ID=$(APP_BUILD_ID) \
	      REPO_PATH=/src$(MIOT_REPO_SUBDIR) \
	      $@ -$(MAKEFLAGS) \
	  "

print-var:
	$(eval _VAL=$$($(VAR)))
	@echo $(_VAL)
