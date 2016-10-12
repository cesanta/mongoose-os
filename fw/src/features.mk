SYS_CONF_SCHEMA += $(MIOT_SRC_PATH)/mg_wifi_config.yaml \
                   $(MIOT_SRC_PATH)/mg_http_config.yaml \
                   $(MIOT_SRC_PATH)/mg_console_config.yaml

ifeq "$(MG_ENABLE_CLUBBY)" "1"
  MIOT_SRCS += clubby.c clubby_channel_ws.c mg_clubby.c
  MIOT_FEATURES += -DMG_ENABLE_CLUBBY -DMG_ENABLE_CLUBBY_API
  SYS_CONF_SCHEMA += $(MIOT_SRC_PATH)/mg_clubby_config.yaml

ifeq "$(MG_ENABLE_CONFIG_SERVICE)" "1"
  MIOT_SRCS += mg_service_config.c mg_service_vars.c
  MIOT_FEATURES += -DMG_ENABLE_CONFIG_SERVICE
endif
ifeq "$(MG_ENABLE_FILESYSTEM_SERVICE)" "1"
  MIOT_SRCS += mg_service_filesystem.c
  MIOT_FEATURES += -DMG_ENABLE_FILESYSTEM_SERVICE
endif
ifeq "$(MG_ENABLE_CLUBBY_UART)" "1"
  MIOT_SRCS += mg_clubby_channel_uart.c
  MIOT_FEATURES += -DMG_ENABLE_CLUBBY_UART
  SYS_CONF_SCHEMA += $(MIOT_SRC_PATH)/mg_clubby_uart_config.yaml
endif

endif # MG_ENABLE_CLUBBY

ifeq "$(MG_ENABLE_DNS_SD)" "1"
  MIOT_SRCS += mg_mdns.c mg_dns_sd.c
  MIOT_FEATURES += -DMG_ENABLE_DNS -DMG_ENABLE_DNS_SERVER -DMG_ENABLE_MDNS -DMG_ENABLE_DNS_SD
  SYS_CONF_SCHEMA += $(MIOT_SRC_PATH)/mg_dns_sd_config.yaml
endif

ifneq "$(MG_ENABLE_UPDATER_POST)$(MG_ENABLE_UPDATER_CLUBBY)" "00"
  SYS_CONF_SCHEMA += $(MIOT_SRC_PATH)/mg_updater_config.yaml
  MIOT_SRCS += mg_updater_common.c
ifeq "$(MG_ENABLE_UPDATER_POST)" "1"
  MIOT_SRCS += mg_updater_post.c
  MIOT_FEATURES += -DMG_ENABLE_UPDATER_POST
endif
ifeq "$(MG_ENABLE_UPDATER_CLUBBY)" "1"
  MIOT_SRCS += mg_updater_clubby.c
  MIOT_FEATURES += -DMG_ENABLE_UPDATER_CLUBBY
endif
endif

# Export all the feature switches.
# This is required for needed make invocations, such as when building POSIX MIOT
# for JS freeze operation.
export MG_ENABLE_CLUBBY
export MG_ENABLE_CLUBBY_UART
export MG_ENABLE_UPDATER_POST
export MG_ENABLE_UPDATER_CLUBBY
export MG_ENABLE_FILESYSTEM_SERVICE
export MG_ENABLE_CONFIG_SERVICE
export MG_ENABLE_DNS_SD
