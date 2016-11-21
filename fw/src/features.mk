SYS_CONF_SCHEMA += $(MIOT_SRC_PATH)/miot_wifi_config.yaml \
                   $(MIOT_SRC_PATH)/miot_http_config.yaml \
                   $(MIOT_SRC_PATH)/miot_console_config.yaml

ifeq "$(MIOT_ENABLE_RPC)" "1"
  MIOT_SRCS += mg_rpc.c mg_rpc_channel_ws.c miot_rpc.c
  MIOT_FEATURES += -DMIOT_ENABLE_RPC -DMIOT_ENABLE_RPC_API
  SYS_CONF_SCHEMA += $(MIOT_SRC_PATH)/miot_rpc_config.yaml

ifeq "$(MIOT_ENABLE_CONFIG_SERVICE)" "1"
  MIOT_SRCS += miot_service_config.c miot_service_vars.c
  MIOT_FEATURES += -DMIOT_ENABLE_CONFIG_SERVICE
endif
ifeq "$(MIOT_ENABLE_FILESYSTEM_SERVICE)" "1"
  MIOT_SRCS += miot_service_filesystem.c
  MIOT_FEATURES += -DMIOT_ENABLE_FILESYSTEM_SERVICE
endif
ifeq "$(MIOT_ENABLE_RPC_UART)" "1"
  MIOT_SRCS += miot_rpc_channel_uart.c
  MIOT_FEATURES += -DMIOT_ENABLE_RPC_UART
  SYS_CONF_SCHEMA += $(MIOT_SRC_PATH)/miot_rpc_uart_config.yaml
endif

endif # MIOT_ENABLE_RPC

ifeq "$(MIOT_ENABLE_DNS_SD)" "1"
  MIOT_SRCS += miot_mdns.c miot_dns_sd.c
  MIOT_FEATURES += -DMG_ENABLE_DNS -DMG_ENABLE_DNS_SERVER -DMIOT_ENABLE_MDNS -DMIOT_ENABLE_DNS_SD
  SYS_CONF_SCHEMA += $(MIOT_SRC_PATH)/miot_dns_sd_config.yaml
endif

ifeq "$(MIOT_ENABLE_I2C)" "1"
  MIOT_SRCS += miot_i2c.c
  MIOT_FEATURES += -DMIOT_ENABLE_I2C
  SYS_CONF_SCHEMA += $(MIOT_SRC_PATH)/miot_i2c_config.yaml
else
  MIOT_FEATURES += -DMIOT_ENABLE_I2C=0
endif

ifeq "$(MIOT_ENABLE_MQTT)" "1"
  MIOT_SRCS += miot_mqtt.c
  MIOT_FEATURES += -DMIOT_ENABLE_MQTT -DMG_ENABLE_MQTT
  SYS_CONF_SCHEMA += $(MIOT_SRC_PATH)/miot_mqtt_config.yaml
else
  MIOT_FEATURES += -DMIOT_ENABLE_MQTT=0 -DMG_ENABLE_MQTT=0
endif

ifneq "$(MIOT_ENABLE_UPDATER)$(MIOT_ENABLE_UPDATER_POST)$(MIOT_ENABLE_UPDATER_RPC)" "000"
  SYS_CONF_SCHEMA += $(MIOT_SRC_PATH)/miot_updater_config.yaml
  MIOT_SRCS += miot_updater_common.c miot_updater_http.c
  MIOT_FEATURES += -DMIOT_ENABLE_UPDATER=1
ifeq "$(MIOT_ENABLE_UPDATER_POST)" "1"
  MIOT_FEATURES += -DMIOT_ENABLE_UPDATER_POST=1
  SYS_CONF_SCHEMA += $(MIOT_SRC_PATH)/miot_updater_post.yaml
endif
ifeq "$(MIOT_ENABLE_UPDATER_RPC)" "1"
  MIOT_SRCS += miot_updater_rpc.c
  MIOT_FEATURES += -DMIOT_ENABLE_UPDATER_RPC=1
endif
endif

# Export all the feature switches.
# This is required for needed make invocations, such as when building POSIX MIOT
# for JS freeze operation.
export MIOT_ENABLE_RPC
export MIOT_ENABLE_RPC_UART
export MIOT_ENABLE_CONFIG_SERVICE
export MIOT_ENABLE_DNS_SD
export MIOT_ENABLE_FILESYSTEM_SERVICE
export MIOT_ENABLE_I2C
export MIOT_ENABLE_JS
export MIOT_ENABLE_MQTT
export MIOT_ENABLE_UPDATER
export MIOT_ENABLE_UPDATER_POST
export MIOT_ENABLE_UPDATER_RPC
