CFLAGS_EXTRA ?=
PYTHON ?= python3

MGOS_SRC_PATH ?= $(MGOS_PATH)/src
MGOS_INCLUDE_PATH ?= $(MGOS_PATH)/include

MGOS_IPATH ?= $(MGOS_PATH) $(MGOS_INCLUDE_PATH) $(MGOS_SRC_PATH) $(MGOS_SRC_PATH)/frozen $(MGOS_SRC_PATH)/umm_malloc
MGOS_VPATH ?= $(MGOS_SRC_PATH) $(MGOS_SRC_PATH)/common $(MGOS_SRC_PATH)/frozen $(MGOS_SRC_PATH)/umm_malloc
MGOS_IFLAGS = $(addprefix -I,$(MGOS_IPATH))
MGOS_CFLAGS ?=
MGOS_SRCS ?=

MG_FEATURES_TINY = -DMG_MODULE_LINES \
                   -DMG_DISABLE_HTTP_KEEP_ALIVE \
                   -DMG_ENABLE_HTTP_SSI=0 \
                   -DMG_ENABLE_TUN=0 \
                   -DMG_ENABLE_HTTP_STREAMING_MULTIPART \
                   -DMG_SSL_IF_MBEDTLS_MAX_FRAGMENT_LEN=1024 \
                   -DMG_MAX_HTTP_REQUEST_SIZE=3072 \
                   -DMG_LOG_DNS_FAILURES \
                   -DMG_MAX_PATH=256 \
                   -DMBUF_SIZE_MULTIPLIER=2 -DMBUF_SIZE_MAX_HEADROOM=128 \
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
