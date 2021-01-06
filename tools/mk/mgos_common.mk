CFLAGS_EXTRA ?=
PYTHON ?= python3

MGOS_SRC_PATH ?= $(MGOS_PATH)/src
MGOS_INCLUDE_PATH ?= $(MGOS_PATH)/include

MGOS_IPATH ?= $(MGOS_PATH) $(MGOS_INCLUDE_PATH) $(MGOS_SRC_PATH) $(MGOS_SRC_PATH)/frozen $(MGOS_SRC_PATH)/umm_malloc
MGOS_VPATH ?= $(MGOS_SRC_PATH) $(MGOS_SRC_PATH)/common $(MGOS_SRC_PATH)/frozen $(MGOS_SRC_PATH)/umm_malloc
MGOS_IFLAGS = $(addprefix -I,$(MGOS_IPATH))
MGOS_CFLAGS ?=
MGOS_SRCS ?=

MG_FEATURES_TINY = -DMBUF_SIZE_MULTIPLIER=2 -DMBUF_SIZE_MAX_HEADROOM=128 \
                   -DMGOS_SDK_BUILD_IMAGE=\"$(MGOS_SDK_BUILD_IMAGE)\"

V ?=
ifeq ("$(V)","1")
Q :=
else
Q := @
endif
vecho := @echo " "

print-var:
	$(eval _VAL=$$($(VAR)))
	@echo $(_VAL)
