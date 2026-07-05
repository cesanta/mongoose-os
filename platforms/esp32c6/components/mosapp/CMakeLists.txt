separate_arguments(app_srcs NATIVE_COMMAND "$ENV{APP_SOURCES}")
idf_component_register(
  SRCS ${app_srcs}
  REQUIRES main
  PRIV_REQUIRES
    app_update
    esp_driver_gpio
    esp_driver_gptimer
    esp_driver_uart
    esp_event
    esp_hal_wdt
    esp_hw_support
    esp_netif
    esp_partition
    esp_rom
    esp_system
    esp_timer
    espcoredump
    freertos
    hal
    lwip
    nvs_flash
    soc
    spi_flash
    vfs
)
