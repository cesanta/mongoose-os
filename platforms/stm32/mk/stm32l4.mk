HSI_VALUE = 16000000
FLASH_S0_SIZE = 2048
MGOS_ROOT_FS_SIZE ?= 131072
SRAM2_BASE_ADDR = 0x10000000
LD_SCRIPT_NO_OTA = $(MGOS_PLATFORM_PATH)/ld/stm32l4_no_ota.ld
LD_SCRIPT_OTA_0 = $(MGOS_PLATFORM_PATH)/ld/stm32l4_ota_0.ld
STM32_CFLAGS += -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 \
                -DSTM32L4 -D__FPU_PRESENT=1 -D__MPU_PRESENT=1 \
                -DMGOS_MAX_NUM_UARTS=6
STM32CUBE_PATH = $(STM32CUBE_L4_PATH)
STM32_IPATH += $(STM32CUBE_PATH)/Drivers/CMSIS/Device/ST/STM32L4xx/Include \
               $(STM32CUBE_PATH)/Drivers/STM32L4xx_HAL_Driver/Inc
STM32_VPATH += $(STM32CUBE_PATH)/Drivers/STM32L4xx_HAL_Driver/Src
STM32_SRCS += \
stm32l4xx_hal_adc.c stm32l4xx_hal_adc_ex.c \
stm32l4xx_hal_can.c \
stm32l4xx_hal_comp.c \
stm32l4xx_hal_cortex.c \
stm32l4xx_hal_crc.c stm32l4xx_hal_crc_ex.c \
stm32l4xx_hal_cryp.c stm32l4xx_hal_cryp_ex.c \
stm32l4xx_hal_dac.c stm32l4xx_hal_dac_ex.c \
stm32l4xx_hal_dcmi.c \
stm32l4xx_hal_dfsdm.c stm32l4xx_hal_dfsdm_ex.c \
stm32l4xx_hal_dma2d.c \
stm32l4xx_hal_dma.c stm32l4xx_hal_dma_ex.c \
stm32l4xx_hal_dsi.c \
stm32l4xx_hal_firewall.c \
stm32l4xx_hal_flash.c stm32l4xx_hal_flash_ex.c stm32l4xx_hal_flash_ramfunc.c \
stm32l4xx_hal_gfxmmu.c \
stm32l4xx_hal_gpio.c \
stm32l4xx_hal_hash.c stm32l4xx_hal_hash_ex.c \
stm32l4xx_hal_hcd.c \
stm32l4xx_hal_i2c.c stm32l4xx_hal_i2c_ex.c \
stm32l4xx_hal_irda.c \
stm32l4xx_hal_iwdg.c \
stm32l4xx_hal_lcd.c \
stm32l4xx_hal_lptim.c \
stm32l4xx_hal_ltdc.c stm32l4xx_hal_ltdc_ex.c \
stm32l4xx_hal_nand.c \
stm32l4xx_hal_nor.c \
stm32l4xx_hal_opamp.c stm32l4xx_hal_opamp_ex.c \
stm32l4xx_hal_ospi.c \
stm32l4xx_hal_pcd.c \
stm32l4xx_hal_pcd_ex.c \
stm32l4xx_hal_pwr.c stm32l4xx_hal_pwr_ex.c \
stm32l4xx_hal_qspi.c \
stm32l4xx_hal_rcc.c stm32l4xx_hal_rcc_ex.c \
stm32l4xx_hal_rng.c \
stm32l4xx_hal_rtc.c stm32l4xx_hal_rtc_ex.c \
stm32l4xx_hal_sai.c stm32l4xx_hal_sai_ex.c \
stm32l4xx_hal_sd.c stm32l4xx_hal_sd_ex.c \
stm32l4xx_hal_smartcard.c stm32l4xx_hal_smartcard_ex.c \
stm32l4xx_hal_smbus.c \
stm32l4xx_hal_spi.c stm32l4xx_hal_spi_ex.c \
stm32l4xx_hal_sram.c \
stm32l4xx_hal_swpmi.c \
stm32l4xx_hal_tim.c stm32l4xx_hal_tim_ex.c \
stm32l4xx_hal_tsc.c \
stm32l4xx_hal_uart.c stm32l4xx_hal_uart_ex.c \
stm32l4xx_hal_usart.c stm32l4xx_hal_usart_ex.c \
stm32l4xx_hal_wwdg.c \
stm32l4xx_ll_adc.c \
stm32l4xx_ll_comp.c \
stm32l4xx_ll_crc.c \
stm32l4xx_ll_crs.c \
stm32l4xx_ll_dac.c \
stm32l4xx_ll_dma2d.c \
stm32l4xx_ll_dma.c \
stm32l4xx_ll_exti.c \
stm32l4xx_ll_fmc.c \
stm32l4xx_ll_gpio.c \
stm32l4xx_ll_i2c.c \
stm32l4xx_ll_lptim.c \
stm32l4xx_ll_lpuart.c \
stm32l4xx_ll_opamp.c \
stm32l4xx_ll_pwr.c \
stm32l4xx_ll_rcc.c \
stm32l4xx_ll_rng.c \
stm32l4xx_ll_rtc.c \
stm32l4xx_ll_sdmmc.c \
stm32l4xx_ll_spi.c \
stm32l4xx_ll_swpmi.c \
stm32l4xx_ll_tim.c \
stm32l4xx_ll_usart.c \
stm32l4xx_ll_usb.c \
stm32l4xx_ll_utils.c

MGOS_SRCS += stm32l4_system.c arm_nsleep100_m4.S
