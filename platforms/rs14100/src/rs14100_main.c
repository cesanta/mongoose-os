/*
 * Copyright (c) 2014-2019 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "common/cs_dbg.h"
#include "common/platform.h"

#include "FreeRTOS.h"
#include "task.h"

#include "arm_exc.h"
#include "mgos_core_dump.h"
#include "mgos_freertos.h"
#include "mgos_gpio.h"
#include "mgos_sys_config.h"
#include "mgos_system.h"
#include "mgos_uart.h"

#include "rs14100_sdk.h"

extern void arm_exc_handler_top(void);
extern const void *flash_int_vectors[60];
static void (*int_vectors[256])(void)
    __attribute__((section(".ram_int_vectors")));

void rs14100_set_int_handler(int irqn, void (*handler)(void)) {
  int_vectors[irqn + 16] = handler;
}

#define ICACHE_BASE 0x20280000
#define ICACHE_CTRL_REG (*((volatile uint32_t *) (ICACHE_BASE + 0x14)))
#define ICACHE_XLATE_REG (*((volatile uint32_t *) (ICACHE_BASE + 0x24)))

uint32_t SystemCoreClockMHZ = 0;

void rs14100_enable_icache(void) {
  // Enable instruction cache.
  RSI_PS_M4ssPeriPowerUp(M4SS_PWRGATE_ULP_ICACHE);
  M4CLK->CLK_ENABLE_SET_REG1_b.ICACHE_CLK_ENABLE_b = 1;
  // M4CLK->CLK_ENABLE_SET_REG1_b.ICACHE_CLK_2X_ENABLE_b = 1;
  // Per HRM, only required for 120MHz+, but we do it always, just in case.
  ICACHE_XLATE_REG |= (1 << 21);
  ICACHE_CTRL_REG = 0x1;  // 4-Way assoc, enable.
}

void rsi_delay_ms(uint32_t delay_ms) {
  mgos_msleep(delay_ms);
}

extern void IRQ074_Handler(void);

enum mgos_init_result mgos_freertos_pre_init(void) {
  return MGOS_INIT_OK;
}

void rs14100_driver_init(void) {
  int32_t status = 0;
  static uint8_t buf[4096];
  static bool s_inited = false;
  static StaticTask_t rsi_driver_task_tcb;
  static StackType_t rsi_driver_task_stack[8192 / sizeof(StackType_t)];

  if (s_inited) return;

  rs14100_set_int_handler(M4_ISR_IRQ, IRQ074_Handler);
  NVIC_SetPriority(M4_ISR_IRQ, 12);
  NVIC_EnableIRQ(M4_ISR_IRQ);

  if ((status = rsi_driver_init(buf, sizeof(buf))) != RSI_SUCCESS) {
    LOG(LL_ERROR, ("RSI %s init failed: %ld", "driver", status));
    return;
  }

  if ((status = rsi_device_init(RSI_LOAD_IMAGE_I_FW)) != RSI_SUCCESS) {
    LOG(LL_ERROR, ("RSI %s init failed: %ld", "device", status));
    return;
  }

  xTaskCreateStatic((TaskFunction_t) rsi_wireless_driver_task, "RSI Drv",
                    sizeof(rsi_driver_task_stack) / sizeof(StackType_t), NULL,
                    15, rsi_driver_task_stack, &rsi_driver_task_tcb);

  rsi_rsp_fw_version_t rsp;
  while (rsi_wlan_get(RSI_FW_VERSION, (uint8_t *) &rsp, sizeof(rsp)) ==
         RSI_ERROR_COMMAND_GIVEN_IN_WRONG_STATE) {
  }
  LOG(LL_INFO, ("NWP version %s", rsp.firmwre_version));

  s_inited = true;
}

void rs14100_wireless_init(void) {
  int32_t status = 0;
  static bool s_inited = false;

  if (s_inited) return;
  // Perform enough init to be able to get MAC address.
  // The rest will be done by the libraries.
  if ((status = rsi_wireless_init(RSI_WLAN_CLIENT_MODE, 0)) != RSI_SUCCESS) {
    LOG(LL_ERROR, ("RSI %s init failed: %ld", "wireless", status));
    return;
  }
  if ((status = rsi_wlan_radio_init()) != RSI_SUCCESS) {
    LOG(LL_ERROR, ("RSI %s init failed: %ld", "radio", status));
  }
  s_inited = true;
}

void device_get_mac_address(uint8_t mac[6]) {
  rs14100_driver_init();
  rs14100_wireless_init();
  if (rsi_wlan_get(RSI_MAC_ADDRESS, mac, 6) != RSI_SUCCESS) {
    memset(mac, 0xaa, 6);
  }
}

void device_set_mac_address(uint8_t mac[6]) {
  rs14100_driver_init();
  rsi_wlan_set(RSI_SET_MAC_ADDRESS, mac, 6);
}

void mgos_dev_system_restart(void) {
  SCB->AIRCR =
      ((0x5FA << SCB_AIRCR_VECTKEY_Pos) | (1 << SCB_AIRCR_SYSRESETREQ_Pos));
  while (1) {
  }
}

IRAM void rs14100_qspi_clock_config(void) {
  // If CPU clock is less than 100 Mhz, make QSPI run in sync with it.
  if (system_clocks.soc_clock <= 100000000) {
    M4CLK->CLK_ENABLE_SET_REG3_b.QSPI_M4_SOC_SYNC_b = 1;
  } else {
    // Otherwise, use SoC PLL. If it's above 100 Mhz, add a divider.
    int div = 1, clk = system_clocks.soc_pll_clock;
    while (clk / div > 100000000) {
      div++;
    }
    M4CLK->CLK_CONFIG_REG1_b.QSPI_CLK_SWALLOW_SEL = 0;
    if (div % 2 == 0) {
      M4CLK->CLK_CONFIG_REG1_b.QSPI_CLK_DIV_FAC = div / 2;
      M4CLK->CLK_CONFIG_REG2_b.QSPI_ODD_DIV_SEL = 0;
    } else {
      M4CLK->CLK_CONFIG_REG1_b.QSPI_CLK_DIV_FAC = div;
      M4CLK->CLK_CONFIG_REG2_b.QSPI_ODD_DIV_SEL = 1;
    }
    M4CLK->CLK_CONFIG_REG1_b.QSPI_CLK_SEL = 3;  // SoC PLL
  }
}

void rs14100_clock_config(uint32_t cpu_freq) {
  RSI_IPMU_CommonConfig();
  RSI_IPMU_PMUCommonConfig();
  RSI_IPMU_M32rc_OscTrimEfuse();
  RSI_IPMU_M20rcOsc_TrimEfuse();
  RSI_IPMU_DBLR32M_TrimEfuse();
  RSI_IPMU_M20roOsc_TrimEfuse();
  RSI_IPMU_RO32khz_TrimEfuse();
  RSI_IPMU_RC16khz_TrimEfuse();
  RSI_IPMU_RC64khz_TrimEfuse();
  RSI_IPMU_RC32khz_TrimEfuse();
  RSI_IPMU_RO_TsEfuse();
  RSI_IPMU_Vbattstatus_TrimEfuse();
  RSI_IPMU_Vbg_Tsbjt_Efuse();
  RSI_IPMU_Auxadcoff_DiffEfuse();
  RSI_IPMU_Auxadcgain_DiffEfuse();
  RSI_IPMU_Auxadcoff_SeEfuse();
  RSI_IPMU_Auxadcgain_SeEfuse();
  RSI_IPMU_Bg_TrimEfuse();
  RSI_IPMU_Blackout_TrimEfuse();
  RSI_IPMU_POCbias_Efuse();
  RSI_IPMU_Buck_TrimEfuse();
  RSI_IPMU_Ldosoc_TrimEfuse();
  RSI_IPMU_Dpwmfreq_TrimEfuse();
  RSI_IPMU_Delvbe_Tsbjt_Efuse();
  RSI_IPMU_Xtal1bias_Efuse();
  RSI_IPMU_Xtal2bias_Efuse();
  RSI_IPMU_InitCalibData();

  RSI_CLK_PeripheralClkEnable3(M4CLK, M4_SOC_CLK_FOR_OTHER_ENABLE);
  RSI_CLK_M4ssRefClkConfig(M4CLK, ULP_32MHZ_RC_CLK);
  RSI_ULPSS_RefClkConfig(ULPSS_ULP_32MHZ_RC_CLK);

  RSI_IPMU_ClockMuxSel(1);
  RSI_PS_FsmLfClkSel(KHZ_RO_CLK_SEL);

  system_clocks.m4ss_ref_clk = DEFAULT_32MHZ_RC_CLOCK;
  system_clocks.ulpss_ref_clk = DEFAULT_32MHZ_RC_CLOCK;
  system_clocks.modem_pll_clock = DEFAULT_MODEM_PLL_CLOCK;
  system_clocks.modem_pll_clock2 = DEFAULT_MODEM_PLL_CLOCK;
  system_clocks.intf_pll_clock = DEFAULT_INTF_PLL_CLOCK;
  system_clocks.rc_32khz_clock = DEFAULT_32KHZ_RC_CLOCK;
  system_clocks.rc_32mhz_clock = DEFAULT_32MHZ_RC_CLOCK;
  system_clocks.ro_20mhz_clock = DEFAULT_20MHZ_RO_CLOCK;
  system_clocks.ro_32khz_clock = DEFAULT_32KHZ_RO_CLOCK;
  system_clocks.xtal_32khz_clock = DEFAULT_32KHZ_XTAL_CLOCK;
  system_clocks.doubler_clock = DEFAULT_DOUBLER_CLOCK;
  system_clocks.rf_ref_clock = DEFAULT_RF_REF_CLOCK;
  system_clocks.mems_ref_clock = DEFAULT_MEMS_REF_CLOCK;
  system_clocks.byp_rc_ref_clock = DEFAULT_32MHZ_RC_CLOCK;
  system_clocks.i2s_pll_clock = DEFAULT_I2S_PLL_CLOCK;

  // 32 MHz RC clk input for the PLLs (all of them, not just SoC).
  RSI_CLK_SocPllRefClkConfig(2);
  RSI_CLK_SetSocPllFreq(M4CLK, cpu_freq, 32000000);

  if (cpu_freq > 90000000) {
    RSI_PS_SetDcDcToHigerVoltage();
    RSI_PS_PowerStateChangePs3toPs4();
  } else {
    RSI_PS_PowerStateChangePs4toPs3();
    RSI_Configure_DCDC_LowerVoltage();
    RSI_PS_SetDcDcToLowerVoltage();
  }
  system_clocks.soc_pll_clock = cpu_freq;

  RSI_CLK_M4SocClkConfig(M4CLK, M4_SOCPLLCLK, 0);
  system_clocks.soc_clock = cpu_freq;

  rs14100_qspi_clock_config();
  SystemCoreClockUpdate();
}

extern void mgos_nsleep100_cal(void);

void SystemCoreClockUpdate(void) {
  SystemCoreClock = system_clocks.soc_clock;
  SystemCoreClockMHZ = SystemCoreClock / 1000000;
#ifndef MGOS_BOOT_BUILD
  mgos_nsleep100_cal();
#endif
}

void rs14100_dump_sram(void) {
  mgos_cd_write_section("SRAM", (void *) SRAM_BASE_ADDR, SRAM_SIZE);
}

uint8_t rs14100_wifi_get_band(void) {
#ifdef MGOS_HAVE_WIFI
  return mgos_sys_config_get_wifi_sta_band();
#endif
  return 0;
}

#ifndef MGOS_BOOT_BUILD
int main(void) {
  /* Move int vectors to RAM. */
  for (int i = 0; i < (int) ARRAY_SIZE(int_vectors); i++) {
    int_vectors[i] = arm_exc_handler_top;
  }
  memcpy(int_vectors, flash_int_vectors, sizeof(flash_int_vectors));
  for (int i = 0; i < (int) ARRAY_SIZE(int_vectors); i++) {
    if (int_vectors[i] == NULL) int_vectors[i] = arm_exc_handler_top;
  }
  SCB->VTOR = (uint32_t) &int_vectors[0];

  SystemInit();
  rs14100_clock_config(180000000 /* cpu_freq */);

  RSI_PS_FsmHfClkSel(FSM_32MHZ_RC);
  RSI_PS_PmuUltraSleepConfig(1U);
  // For some reason cache just doesn't help, even slows things down a bit.
  // rs14100_enable_icache();

  // Set up region 7 (highest priority) to block first megabyte of SRAM (to
  // catch NULL derefs).
  MPU->RNR = 7;
  MPU->RBAR = 0;
  MPU->RASR = MPU_RASR_ENABLE_Msk |
              (19 << MPU_RASR_SIZE_Pos) |  // 1M bytes (2^(19+1)).
              (0 << MPU_RASR_AP_Pos) |     // No RW access.
              (1 << MPU_RASR_XN_Pos);      // No code execution.
  // We run in privileged mode all the time so PRIVDEFENA acts as
  // "allow everything else".
  MPU->CTRL = MPU_CTRL_ENABLE_Msk | MPU_CTRL_PRIVDEFENA_Msk;

  mgos_cd_register_section_writer(arm_exc_dump_regs);
  mgos_cd_register_section_writer(rs14100_dump_sram);

  NVIC_SetPriorityGrouping(0);
  NVIC_SetPriority(MemoryManagement_IRQn, 0);
  NVIC_SetPriority(BusFault_IRQn, 0);
  NVIC_SetPriority(UsageFault_IRQn, 0);
  NVIC_SetPriority(DebugMonitor_IRQn, 0);
  rs14100_set_int_handler(SVCall_IRQn, vPortSVCHandler);
  rs14100_set_int_handler(PendSV_IRQn, xPortPendSVHandler);
  rs14100_set_int_handler(SysTick_IRQn, xPortSysTickHandler);
  mgos_freertos_run_mgos_task(true /* start_scheduler */);
  /* not reached */
  abort();
}
#endif  // MGOS_BOOT_BUILD
