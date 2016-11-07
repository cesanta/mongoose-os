# Common rules for "outer" Makefiles, i.e. the ones that invoke Docker.
#
# App must define its name (APP) and path to MIOT repo (MIOT_PATH).

MAKEFLAGS += --warn-undefined-variables
APP_VERSION ?=
APP_BUILD_ID ?=
PYTHON ?= python
PLATFORM ?=
DOCKER_EXTRA ?=

DOCKER_APP_PATH = /app
DOCKER_MIOT_PATH = /mongoose-iot

APP_PATH = $(CURDIR)
MIOT_PATH_ABS=$(abspath $(MIOT_PATH))

SDK_VERSION ?= $(shell cat $(MIOT_PATH)/fw/platforms/$(APP_PLATFORM)/sdk.version)

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
      -C $(MAKE_APP_PATH) -f Makefile.build \
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

# We're outside of the container, so, invoke docker properly.
# Note about mounts: we mount repo to a stable path (/app) as well as the
# original path outside the container, whatever it may be, so that absolute path
# references continue to work (e.g. Git submodules are known to use abs. paths).
MAKE_APP_PATH=$(DOCKER_APP_PATH)
MAKE_MIOT_PATH=$(DOCKER_MIOT_PATH)
INNER_MAKE=make
all clean:
	@docker run --rm -i --tty=$T \
	  -v $(APP_PATH):$(DOCKER_APP_PATH) \
	  -v $(MIOT_PATH_ABS):$(DOCKER_MIOT_PATH) \
	  -v $(MIOT_PATH_ABS):$(MIOT_PATH_ABS) \
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
