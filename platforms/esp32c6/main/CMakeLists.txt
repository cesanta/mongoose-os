separate_arguments(gen_srcs NATIVE_COMMAND "$ENV{GEN_SOURCES}")
separate_arguments(mgos_srcs NATIVE_COMMAND "$ENV{MGOS_SOURCES}")
idf_component_register(
  INCLUDE_DIRS "."
  SRCS ${gen_srcs} ${mgos_srcs}
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
# In ffi exports file we use fake signatures: void func(void), and it conflicts
# with the builtin functions like fopen, etc.
set_source_files_properties(${FFI_EXPORTS_C} PROPERTIES COMPILE_FLAGS -fno-builtin)
