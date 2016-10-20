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
PLATFORM ?=
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

INNER_MAKE?=$(MAKE)

# `make` command, which will be invoked either directly or inside the newly
# created docker container. It uses MAKE_REPO_ABS_PATH, which will be set later
MAKE_CMD=$(INNER_MAKE) -j4 \
      -C $(MAKE_REPO_ABS_PATH)/$(APP_SUBDIR) -f Makefile.build \
      APP=$(APP) \
      APP_VERSION=$(APP_VERSION) \
      APP_BUILD_ID=$(APP_BUILD_ID) \
      REPO_PATH=$(MAKE_REPO_ABS_PATH)/$(MIOT_REPO_SUBDIR) \
      PLATFORM=$(PLATFORM) \
      $@ -$(MAKEFLAGS)

# define targets "all" and "clean" differently, depending on whether we're
# inside our docker container
ifeq ("$(MIOT_SDK_REVISION)","")

# We're outside of the container, so, invoke docker properly.
# Note about mounts: we mount repo to a stable path (/src) as well as the
# original path outside the container, whatever it may be, so that absolute path
# references continue to work (e.g. Git submodules are known to use abs. paths).
MAKE_REPO_ABS_PATH=/src
INNER_MAKE=make
all clean:
	@docker run --rm -i --tty=$T \
	  -v $(REPO_ABS_PATH):/src \
	  -v $(REPO_ABS_PATH):$(REPO_ABS_PATH) \
	  $(DOCKER_EXTRA) $(SDK_VERSION) \
	  /bin/bash -c "\
	    nice $(MAKE_CMD) \
	  "

else

# We're already inside of container, so, invoke `make` directly
MAKE_REPO_ABS_PATH=$(REPO_ABS_PATH)
all clean:
	@$(MAKE_CMD)

endif

print-var:
	$(eval _VAL=$$($(VAR)))
	@echo $(_VAL)
