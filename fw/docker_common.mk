# Common rules for "outer" Makefiles, i.e. the ones that invoke Docker.
#
# App must define its name (APP) and path to MIOT repo (MIOT_PATH).

MAKEFLAGS += --warn-undefined-variables
APP_VERSION ?=
APP_BUILD_ID ?=
PYTHON ?= python
PLATFORM ?=
DOCKER_EXTRA ?=
MAKEFILE_BUILD ?= $(MAKE_MIOT_PATH)/fw/platforms/$(APP_PLATFORM)/Makefile.build

DOCKER_APP_PATH = /app
DOCKER_MIOT_PATH = /mongoose-iot

APP_PATH = $(CURDIR)
MIOT_PATH_ABS=$(abspath $(MIOT_PATH))

SDK_VERSION ?= $(shell cat $(MIOT_PATH)/fw/platforms/$(APP_PLATFORM)/sdk.version)
REPO_PATH ?= $(shell git rev-parse --show-toplevel 2> /dev/null)
ifeq ($(REPO_PATH),)
  # We're outside of any git repository: will just mount the application path
  APP_MOUNT_PATH = $(APP_PATH)
  APP_SUBDIR =
else
  # We're inside some git repo: will mount the root of this repo, and remember
  # the app's subdir inside it.
  APP_MOUNT_PATH = $(REPO_PATH)
  APP_SUBDIR = $(subst $(REPO_PATH),,$(APP_PATH))
endif

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
# created docker container. It uses MAKE_APP_PATH and MAKE_MIOT_PATH which
# will be set later.
#
# NOTE that $(MAKEFILES) should be given _before_ our own variable definitions,
# because MAKEFLAGS contains MIOT_PATH which we want to override.
MAKE_CMD=$(INNER_MAKE) -j4 \
      -C $(MAKE_APP_PATH) -f $(MAKEFILE_BUILD) \
      -$(MAKEFLAGS) \
      APP=$(APP) \
      APP_VERSION=$(APP_VERSION) \
      APP_BUILD_ID=$(APP_BUILD_ID) \
      MIOT_PATH=$(MAKE_MIOT_PATH) \
      PLATFORM=$(PLATFORM) \
      $@

# define targets "all" and "clean" differently, depending on whether we're
# inside our docker container
ifeq ("$(MIOT_SDK_REVISION)","")

# On Windows and Mac, run container as root since volume sharing on those OSes
# doesn't play nice with unprivileged user.
#
# On other OSes, run it as the current user.
DOCKER_USER_ARG =
ifneq ($(OS),Windows_NT)
UNAME_S := $(shell uname -s)
ifneq ($(UNAME_S),Darwin)
DOCKER_USER_ARG = --user $$(id -u):$$(id -u)
endif
endif

# We're outside of the container, so, invoke docker properly.
# Note about mounts: we mount repo to a stable path (/app) as well as the
# original path outside the container, whatever it may be, so that absolute path
# references continue to work (e.g. Git submodules are known to use abs. paths).
MAKE_APP_PATH=$(DOCKER_APP_PATH)$(APP_SUBDIR)
MAKE_MIOT_PATH=$(DOCKER_MIOT_PATH)
INNER_MAKE=make
all clean menuconfig:
	@docker run --rm -i --tty=$T \
	  -v $(APP_MOUNT_PATH):$(DOCKER_APP_PATH) \
	  -v $(MIOT_PATH_ABS):$(DOCKER_MIOT_PATH) \
	  -v $(MIOT_PATH_ABS):$(MIOT_PATH_ABS) \
	  $(DOCKER_USER_ARG) \
	  $(DOCKER_EXTRA) $(SDK_VERSION) \
	  /bin/bash -c "\
	    nice $(MAKE_CMD) \
	  "

else

# We're already inside of container, so, invoke `make` directly
MAKE_APP_PATH=$(APP_PATH)
MAKE_MIOT_PATH=$(MIOT_PATH_ABS)
all clean:
	@$(MAKE_CMD)

endif

print-var:
	$(eval _VAL=$$($(VAR)))
	@echo $(_VAL)
