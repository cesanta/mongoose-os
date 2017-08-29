MGOS_ENABLE_BITBANG ?= 1
MGOS_ENABLE_DEBUG_UDP ?= 1
MGOS_ENABLE_MDNS ?= 0
MGOS_ENABLE_SNTP ?= 1
MGOS_ENABLE_SYS_SERVICE ?= 1
MGOS_ENABLE_UPDATER ?= 0

MGOS_DEBUG_UART ?= 0
MGOS_EARLY_DEBUG_LEVEL ?= LL_INFO
MGOS_DEBUG_UART_BAUD_RATE ?= 115200
MGOS_SRCS += mgos_debug.c mgos_net.c

MGOS_FEATURES += -DMGOS_DEBUG_UART=$(MGOS_DEBUG_UART) \
                 -DMGOS_EARLY_DEBUG_LEVEL=$(MGOS_EARLY_DEBUG_LEVEL) \
                 -DMGOS_DEBUG_UART_BAUD_RATE=$(MGOS_DEBUG_UART_BAUD_RATE) \
                 -DMG_ENABLE_CALLBACK_USERDATA

ifdef MGOS_HAVE_ATCA
  ATCA_PATH ?= $(MGOS_PATH)/third_party/cryptoauthlib
  ATCA_LIB = $(BUILD_DIR)/libatca.a

  MGOS_FEATURES += -I$(ATCA_PATH)/lib

$(BUILD_DIR)/atca/libatca.a:
	$(Q) mkdir -p $(BUILD_DIR)/atca
	$(Q) make -C $(ATCA_PATH)/lib \
		CC=$(CC) AR=$(AR) BUILD_DIR=$(BUILD_DIR)/atca \
	  CFLAGS="$(CFLAGS)"

$(ATCA_LIB): $(BUILD_DIR)/atca/libatca.a
	$(Q) cp $< $@
	$(Q) $(OBJCOPY) --rename-section .rodata=.irom0.text $@
	$(Q) $(OBJCOPY) --rename-section .rodata.str1.1=.irom0.text $@
else
  ATCA_LIB =
endif

ifeq "$(MGOS_ENABLE_DEBUG_UDP)" "1"
  MGOS_FEATURES += -DMGOS_ENABLE_DEBUG_UDP
  SYS_CONF_SCHEMA += $(MGOS_SRC_PATH)/mgos_debug_udp_config.yaml
endif

ifeq "$(MGOS_ENABLE_BITBANG)" "1"
  MGOS_SRCS += mgos_bitbang.c
  MGOS_FEATURES += -DMGOS_ENABLE_BITBANG
endif

ifeq "$(MGOS_ENABLE_MDNS)" "1"
  MGOS_SRCS += mgos_mdns.c
  MGOS_FEATURES += -DMG_ENABLE_DNS -DMG_ENABLE_DNS_SERVER -DMGOS_ENABLE_MDNS
endif

ifeq "$(MGOS_ENABLE_SNTP)" "1"
  MGOS_SRCS += mgos_sntp.c
  MGOS_FEATURES += -DMG_ENABLE_SNTP -DMGOS_ENABLE_SNTP
  SYS_CONF_SCHEMA += $(MGOS_SRC_PATH)/mgos_sntp_config.yaml
endif

ifeq "$(MGOS_ENABLE_UPDATER)" "1"
  SYS_CONF_SCHEMA += $(MGOS_SRC_PATH)/mgos_updater_config.yaml
  MGOS_SRCS += mgos_updater_common.c
  MGOS_FEATURES += -DMGOS_ENABLE_UPDATER
endif

# Export all the feature switches.
# This is required for needed make invocations (i.e. ESP32 IDF)
export MGOS_ENABLE_BITBANG
export MGOS_ENABLE_DEBUG_UDP
export MGOS_ENABLE_SNTP
export MGOS_ENABLE_SYS_SERVICE
export MGOS_ENABLE_UPDATER
