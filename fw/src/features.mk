SYS_CONF_SCHEMA += $(MIOT_SRC_PATH)/miot_wifi_config.yaml \
                   $(MIOT_SRC_PATH)/miot_http_config.yaml \
                   $(MIOT_SRC_PATH)/miot_console_config.yaml

ifeq "$(MG_ENABLE_RPC)" "1"
  MIOT_SRCS += mg_rpc.c mg_rpc_channel_ws.c miot_rpc.c
  MIOT_FEATURES += -DMG_ENABLE_RPC -DMG_ENABLE_RPC_API
  SYS_CONF_SCHEMA += $(MIOT_SRC_PATH)/miot_rpc_config.yaml

ifeq "$(MG_ENABLE_CONFIG_SERVICE)" "1"
  MIOT_SRCS += miot_service_config.c miot_service_vars.c
  MIOT_FEATURES += -DMG_ENABLE_CONFIG_SERVICE
endif
ifeq "$(MG_ENABLE_FILESYSTEM_SERVICE)" "1"
  MIOT_SRCS += miot_service_filesystem.c
  MIOT_FEATURES += -DMG_ENABLE_FILESYSTEM_SERVICE
endif
ifeq "$(MG_ENABLE_RPC_UART)" "1"
  MIOT_SRCS += miot_rpc_channel_uart.c
  MIOT_FEATURES += -DMG_ENABLE_RPC_UART
  SYS_CONF_SCHEMA += $(MIOT_SRC_PATH)/miot_rpc_uart_config.yaml
endif

endif # MG_ENABLE_RPC

ifeq "$(MG_ENABLE_DNS_SD)" "1"
  MIOT_SRCS += miot_mdns.c miot_dns_sd.c
  MIOT_FEATURES += -DMG_ENABLE_DNS -DMG_ENABLE_DNS_SERVER -DMG_ENABLE_MDNS -DMG_ENABLE_DNS_SD
  SYS_CONF_SCHEMA += $(MIOT_SRC_PATH)/miot_dns_sd_config.yaml
endif

ifeq "$(MG_ENABLE_MQTT)" "1"
  MIOT_SRCS += miot_mqtt.c
  MIOT_FEATURES += -DMG_ENABLE_MQTT
  SYS_CONF_SCHEMA += $(MIOT_SRC_PATH)/miot_mqtt_config.yaml
else
  MIOT_FEATURES += -DMG_ENABLE_MQTT=0
endif

ifneq "$(MG_ENABLE_UPDATER)$(MG_ENABLE_UPDATER_POST)$(MG_ENABLE_UPDATER_RPC)" "000"
  SYS_CONF_SCHEMA += $(MIOT_SRC_PATH)/miot_updater_config.yaml
  MIOT_SRCS += miot_updater_common.c miot_updater_http.c
  MIOT_FEATURES += -DMG_ENABLE_UPDATER=1
ifeq "$(MG_ENABLE_UPDATER_POST)" "1"
  MIOT_FEATURES += -DMG_ENABLE_UPDATER_POST=1
  SYS_CONF_SCHEMA += $(MIOT_SRC_PATH)/miot_updater_post.yaml
endif
ifeq "$(MG_ENABLE_UPDATER_RPC)" "1"
  MIOT_SRCS += miot_updater_rpc.c
  MIOT_FEATURES += -DMG_ENABLE_UPDATER_RPC=1
endif
endif

# Export all the feature switches.
# This is required for needed make invocations, such as when building POSIX MIOT
# for JS freeze operation.
export MG_ENABLE_RPC
export MG_ENABLE_RPC_UART
export MG_ENABLE_CONFIG_SERVICE
export MG_ENABLE_DNS_SD
export MG_ENABLE_FILESYSTEM_SERVICE
export MG_ENABLE_JS
export MG_ENABLE_MQTT
export MG_ENABLE_UPDATER
export MG_ENABLE_UPDATER_POST
export MG_ENABLE_UPDATER_RPC
