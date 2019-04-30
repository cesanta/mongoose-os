/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
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

/*
 * Cesanta note: this is inspired by
 * STM32Cube_FW_F7_V1.8.0/Projects/STM32F769I-Discovery/Applications/LwIP/LwIP_HTTP_Server_Socket_RTOS/Src/ethernetif.c
 * with substnatial additions and modifications.
 */

/**
  ******************************************************************************
  * @file    ethernetif.c
  * @author  MCD Application Team
  * @brief   This file implements Ethernet network interface drivers for lwIP
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V.
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice,
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other
  *    contributors to this software may be used to endorse or promote products
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under
  *    this license is void and will automatically terminate your rights under
  *    this license.
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

#include "stm32_eth_netif.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "common/cs_dbg.h"

#include "stm32f7xx_hal.h"
#include "lwip/dhcp.h"
#include "lwip/opt.h"
#include "lwip/prot/dhcp.h"
#include "lwip/timeouts.h"
#include "netif/ethernet.h"
#include "netif/etharp.h"

#include "mgos_eth.h"
#include "mgos_gpio.h"
#include "mgos_net_hal.h"

#include "stm32_eth_phy.h"
#include "stm32_system.h"

#include "cmsis_os.h"

/* The time to block waiting for input. */
#define TIME_WAITING_FOR_INPUT (100)
/* Stack size of the interface thread */
#define INTERFACE_THREAD_STACK_SIZE (1024) /* x4 */

/* Define those to better describe your network interface. */
#define IFNAME0 's'
#define IFNAME1 't'

//#undef LL_VERBOSE_DEBUG
//#define LL_VERBOSE_DEBUG LL_DEBUG

/*
 * Note: DMA desxriptors must be in non-cacheable RAM.
 */
ETH_DMADescTypeDef rx_descr[ETH_RXBUFNB] __attribute__((section(".nocache")));
ETH_DMADescTypeDef tx_descr[ETH_TXBUFNB] __attribute__((section(".nocache")));

struct stm32_eth_netif_state {
  struct netif *netif;
  struct mgos_eth_phy_opts phy_opts;

  ETH_HandleTypeDef heth;
  /* XXX(rojer): Not sure why this is necessary, but otherwise first 16 bytes
   * of the first buffer get zeroed. Or rather, not DMA'd correctly.
   * Needs nmore investigation. */
  uint8_t pad[16];
  uint8_t rx_buf[ETH_RXBUFNB][ETH_RX_BUF_SIZE];
  uint8_t tx_buf[ETH_TXBUFNB][ETH_TX_BUF_SIZE];
  osSemaphoreDef_t sem;
  osSemaphoreId sem_id;
};

struct stm32_eth_netif_state *s_state = NULL;

static void stm32_eth_netif_task(void const *arg);

void HAL_ETH_MspInit(ETH_HandleTypeDef *heth) {
  __HAL_RCC_ETH_CLK_ENABLE();

#ifdef STM32_ETH_MAC_PIN_REF_CLK
  mgos_gpio_set_mode(STM32_ETH_MAC_PIN_REF_CLK, MGOS_GPIO_MODE_OUTPUT);
  mgos_gpio_set_mode(STM32_ETH_MAC_PIN_MDC, MGOS_GPIO_MODE_OUTPUT);
  mgos_gpio_set_mode(STM32_ETH_MAC_PIN_MDIO, MGOS_GPIO_MODE_OUTPUT);
  mgos_gpio_set_mode(STM32_ETH_MAC_PIN_CRS_DV, MGOS_GPIO_MODE_OUTPUT);
  mgos_gpio_set_mode(STM32_ETH_MAC_PIN_RXD0, MGOS_GPIO_MODE_INPUT);
  mgos_gpio_set_mode(STM32_ETH_MAC_PIN_RXD1, MGOS_GPIO_MODE_INPUT);
  mgos_gpio_set_mode(STM32_ETH_MAC_PIN_TXD0, MGOS_GPIO_MODE_OUTPUT);
  mgos_gpio_set_mode(STM32_ETH_MAC_PIN_TXD1, MGOS_GPIO_MODE_OUTPUT);
  mgos_gpio_set_mode(STM32_ETH_MAC_PIN_TXD_EN, MGOS_GPIO_MODE_OUTPUT);
#else
#error Ethernet MAC pins are not defined. Please define STM32_ETH_MAC_PIN_xxx macros.
#endif

  heth->Init.MediaInterface = ETH_MEDIA_INTERFACE_RMII;

  stm32_eth_phy_init(heth);
}

void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *heth) {
  if (s_state != NULL) osSemaphoreRelease(s_state->sem_id);
  (void) heth;
}

/**
  * @brief This function should do the actual transmission of the packet. The
  *packet is
  * contained in the pbuf that is passed to the function. This pbuf
  * might be chained.
  *
  * @param netif the lwip network interface structure for this ethernetif
  * @param p the MAC packet to send (e.g. IP packet including MAC addresses and
  *        type)
  * @return ERR_OK if the packet could be sent
  *         an err_t value if the packet couldn't be sent
  *
  * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
  *       strange results. You might consider waiting for space in the DMA queue
  *       to become available since the stack doesn't retry to send a packet
  *       dropped because of memory failure (except for the TCP timers).
  */
static err_t low_level_output(struct netif *netif, struct pbuf *p) {
  struct stm32_eth_netif_state *state =
      (struct stm32_eth_netif_state *) netif->state;
  ETH_HandleTypeDef *heth = &state->heth;

  err_t errval;
  struct pbuf *q;
  uint8_t *buffer = (uint8_t *) (state->heth.TxDesc->Buffer1Addr);
  __IO ETH_DMADescTypeDef *DmaTxDesc;
  uint32_t framelength = 0;
  uint32_t bufferoffset = 0;
  uint32_t byteslefttocopy = 0;
  uint32_t payloadoffset = 0;

  DmaTxDesc = heth->TxDesc;
  bufferoffset = 0;

  /* copy frame from pbufs to driver buffers */
  for (q = p; q != NULL; q = q->next) {
    /* Is this buffer available? If not, goto error */
    if ((DmaTxDesc->Status & ETH_DMATXDESC_OWN) != (uint32_t) RESET) {
      errval = ERR_USE;
      goto error;
    }

    /* Get bytes in current lwIP buffer */
    byteslefttocopy = q->len;
    payloadoffset = 0;

    /* Check if the length of data to copy is bigger than Tx buffer size*/
    while ((byteslefttocopy + bufferoffset) > ETH_TX_BUF_SIZE) {
      /* Copy data to Tx buffer*/
      memcpy((uint8_t *) ((uint8_t *) buffer + bufferoffset),
             (uint8_t *) ((uint8_t *) q->payload + payloadoffset),
             (ETH_TX_BUF_SIZE - bufferoffset));

      /* Point to next descriptor */
      DmaTxDesc = (ETH_DMADescTypeDef *) (DmaTxDesc->Buffer2NextDescAddr);

      /* Check if the buffer is available */
      if ((DmaTxDesc->Status & ETH_DMATXDESC_OWN) != (uint32_t) RESET) {
        errval = ERR_USE;
        goto error;
      }

      buffer = (uint8_t *) (DmaTxDesc->Buffer1Addr);

      byteslefttocopy = byteslefttocopy - (ETH_TX_BUF_SIZE - bufferoffset);
      payloadoffset = payloadoffset + (ETH_TX_BUF_SIZE - bufferoffset);
      framelength = framelength + (ETH_TX_BUF_SIZE - bufferoffset);
      bufferoffset = 0;
    }

    /* Copy the remaining bytes */
    memcpy((uint8_t *) ((uint8_t *) buffer + bufferoffset),
           (uint8_t *) ((uint8_t *) q->payload + payloadoffset),
           byteslefttocopy);
    bufferoffset = bufferoffset + byteslefttocopy;
    framelength = framelength + byteslefttocopy;
  }

  /* Clean and Invalidate data cache */
  if (SCB->CCR & SCB_CCR_DC_Msk) SCB_CleanInvalidateDCache();

  /* Prepare transmit descriptors to give to DMA */
  HAL_ETH_TransmitFrame(heth, framelength);

  errval = ERR_OK;

error:
  LOG(LL_VERBOSE_DEBUG, ("Eth TX p %p %d %d", p, (int) p->tot_len, errval));

  /* When Transmit Underflow flag is set, clear it and issue a Transmit Poll
   * Demand to resume transmission */
  if ((heth->Instance->DMASR & ETH_DMASR_TUS) != (uint32_t) RESET) {
    /* Clear TUS ETHERNET DMA flag */
    heth->Instance->DMASR = ETH_DMASR_TUS;

    /* Resume DMA transmission*/
    heth->Instance->DMATPDR = 0;
  }
  return errval;
}

/**
  * @brief Should allocate a pbuf and transfer the bytes of the incoming
  * packet from the interface into the pbuf.
  *
  * @param netif the lwip network interface structure for this ethernetif
  * @return a pbuf filled with the received packet (including MAC header)
  *         NULL on memory error
  */
static struct pbuf *low_level_input(struct netif *netif) {
  struct stm32_eth_netif_state *state =
      (struct stm32_eth_netif_state *) netif->state;
  ETH_HandleTypeDef *heth = &state->heth;

  struct pbuf *p = NULL, *q = NULL;
  uint16_t len = 0;
  uint8_t *buffer;
  __IO ETH_DMADescTypeDef *dmarxdesc;
  uint32_t bufferoffset = 0;
  uint32_t payloadoffset = 0;
  uint32_t byteslefttocopy = 0;
  uint32_t i = 0;

  /* get received frame */
  if (HAL_ETH_GetReceivedFrame_IT(heth) != HAL_OK) return NULL;

  /* Obtain the size of the packet and put it into the "len" variable. */
  len = heth->RxFrameInfos.length;
  buffer = (uint8_t *) heth->RxFrameInfos.buffer;

  if (len > 0) {
    /* We allocate a pbuf chain of pbufs from the Lwip buffer pool */
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
  }

  /* Clean and Invalidate data cache */
  if (SCB->CCR & SCB_CCR_DC_Msk) SCB_CleanInvalidateDCache();

  if (p != NULL) {
    dmarxdesc = heth->RxFrameInfos.FSRxDesc;
    bufferoffset = 0;

    for (q = p; q != NULL; q = q->next) {
      byteslefttocopy = q->len;
      payloadoffset = 0;

      /* Check if the length of bytes to copy in current pbuf is bigger than Rx
       * buffer size */
      while ((byteslefttocopy + bufferoffset) > ETH_RX_BUF_SIZE) {
        /* Copy data to pbuf */
        memcpy((uint8_t *) ((uint8_t *) q->payload + payloadoffset),
               (uint8_t *) ((uint8_t *) buffer + bufferoffset),
               (ETH_RX_BUF_SIZE - bufferoffset));

        /* Point to next descriptor */
        dmarxdesc = (ETH_DMADescTypeDef *) (dmarxdesc->Buffer2NextDescAddr);
        buffer = (uint8_t *) (dmarxdesc->Buffer1Addr);

        byteslefttocopy = byteslefttocopy - (ETH_RX_BUF_SIZE - bufferoffset);
        payloadoffset = payloadoffset + (ETH_RX_BUF_SIZE - bufferoffset);
        bufferoffset = 0;
      }

      /* Copy remaining data in pbuf */
      memcpy((uint8_t *) ((uint8_t *) q->payload + payloadoffset),
             (uint8_t *) ((uint8_t *) buffer + bufferoffset), byteslefttocopy);
      bufferoffset = bufferoffset + byteslefttocopy;
    }
  }

  /* Release descriptors to DMA */
  /* Point to first descriptor */
  dmarxdesc = heth->RxFrameInfos.FSRxDesc;
  /* Set Own bit in Rx descriptors: gives the buffers back to DMA */
  for (i = 0; i < heth->RxFrameInfos.SegCount; i++) {
    dmarxdesc->Status |= ETH_DMARXDESC_OWN;
    dmarxdesc = (ETH_DMADescTypeDef *) (dmarxdesc->Buffer2NextDescAddr);
  }

  /* Clear Segment_Count */
  heth->RxFrameInfos.SegCount = 0;

  /* When Rx Buffer unavailable flag is set: clear it and resume reception */
  if ((heth->Instance->DMASR & ETH_DMASR_RBUS) != (uint32_t) RESET) {
    /* Clear RBUS ETHERNET DMA flag */
    heth->Instance->DMASR = ETH_DMASR_RBUS;
    /* Resume DMA reception */
    heth->Instance->DMARPDR = 0;
  }
  return p;
}

static void stm32_eth_netif_status(struct netif *netif,
                                   struct stm32_eth_netif_state *state) {
  static bool s_link_up = false;

  ETH_HandleTypeDef *heth = &state->heth;
  struct stm32_eth_phy_status status;
  if (!stm32_eth_phy_get_status(heth, &status)) {
    return;
  }
  LOG(LL_VERBOSE_DEBUG,
      ("Eth status: link %s, speed %s, duplex %s, "
       "autoneg %s, ar.speed %s, ar.duplex %s",
       (status.link_up ? "up" : "down"), mgos_eth_speed_str(status.opts.speed),
       mgos_eth_duplex_str(status.opts.duplex),
       (status.opts.autoneg_on ? (status.ar.complete ? "done" : "in progress")
                               : "off"),
       mgos_eth_speed_str(status.ar.speed),
       mgos_eth_duplex_str(status.ar.duplex)));
  struct dhcp *dhcp = ((struct dhcp *) netif_get_client_data(
      netif, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP));
  if (state->phy_opts.autoneg_on != status.opts.autoneg_on) {
    status.opts.autoneg_on = state->phy_opts.autoneg_on;
    stm32_eth_phy_set_opts(heth, &status.opts);
  } else if (status.link_up != s_link_up) {
    s_link_up = status.link_up;
    if (s_link_up) {
      /* If we finished autoneg, sync PHY settings to autoneg results. */
      if (status.opts.autoneg_on && status.ar.complete) {
        status.opts.speed = status.ar.speed;
        status.opts.duplex = status.ar.duplex;
        stm32_eth_phy_set_opts(heth, &status.opts);
      }

      /* Sync MAC settings to PHY settings. */
      uint32_t maccr = heth->Instance->MACCR;
      maccr &= ~(ETH_SPEED_100M | ETH_MODE_FULLDUPLEX);
      heth->Init.Speed =
          (status.opts.speed == MGOS_ETH_SPEED_100M ? ETH_SPEED_100M
                                                    : ETH_SPEED_10M);
      heth->Init.DuplexMode =
          (status.opts.duplex == MGOS_ETH_DUPLEX_FULL ? ETH_MODE_FULLDUPLEX
                                                      : ETH_MODE_HALFDUPLEX);
      maccr |= (heth->Init.Speed | heth->Init.DuplexMode);
      heth->Instance->MACCR = maccr;

      LOG(LL_INFO,
          ("ETH: Link up, %dMbps, %s",
           (heth->Init.Speed == ETH_SPEED_100M ? 100 : 10),
           (heth->Init.DuplexMode == ETH_MODE_FULLDUPLEX ? "full-duplex"
                                                         : "half-duplex")));
      netif_set_link_up(netif);
      mgos_net_dev_event_cb(MGOS_NET_IF_TYPE_ETHERNET, netif->num,
                            MGOS_NET_EV_CONNECTED);
      if (dhcp == NULL) {
        mgos_net_dev_event_cb(MGOS_NET_IF_TYPE_ETHERNET, netif->num,
                              MGOS_NET_EV_IP_ACQUIRED);
      }
    } else {
      LOG(LL_INFO, ("ETH: Link down"));
      netif_set_link_down(netif);
      mgos_net_dev_event_cb(MGOS_NET_IF_TYPE_ETHERNET, netif->num,
                            MGOS_NET_EV_DISCONNECTED);
    }
  }
  if (dhcp != NULL && s_link_up) {
    static uint8_t s_prev_dhcp_state = DHCP_STATE_OFF;
    if (dhcp->state != s_prev_dhcp_state) {
      LOG(LL_DEBUG, ("DHCP state %d -> %d", s_prev_dhcp_state, dhcp->state));
      s_prev_dhcp_state = dhcp->state;
      if (dhcp->state == DHCP_STATE_BOUND) {
        if (!ip4_addr_isany_val(netif->gw)) {
          netif_set_default(netif);
        }
        mgos_net_dev_event_cb(MGOS_NET_IF_TYPE_ETHERNET, netif->num,
                              MGOS_NET_EV_IP_ACQUIRED);
      }
    }
  }
}

/**
  * @brief This function is the stm32_eth_netif_task, it is processed when a
  * packet is ready to be read from the interface. It uses the function
  * low_level_input() that should handle the actual reception of bytes from
  * the network interface. Then the type of the received packet is determined
  * and the appropriate input function is called.
  *
  * @param netif the lwip network interface structure for this ethernetif
  */
static void stm32_eth_netif_task(void const *argument) {
  struct pbuf *p;
  struct netif *netif = (struct netif *) argument;
  struct stm32_eth_netif_state *state =
      (struct stm32_eth_netif_state *) netif->state;

  for (;;) {
    if (osSemaphoreAcquire(state->sem_id, TIME_WAITING_FOR_INPUT) == osOK) {
      do {
        p = low_level_input(netif);
        if (p != NULL) {
          LOG(LL_VERBOSE_DEBUG,
              ("Eth RX %p %d %d", p->payload, (int) p->tot_len, (int) p->len));
          if (netif->input(p, netif) != ERR_OK) {
            pbuf_free(p);
          }
        }
      } while (p != NULL);
    } else {
      stm32_eth_netif_status(netif, state);
    }
  }
}

void stm32_eth_int_handler(void) {
  // mgos_gpio_write(LED1, 1);
  if (s_state != NULL) HAL_ETH_IRQHandler(&s_state->heth);
}

/**
  * @brief Should be called at the beginning of the program to set up the
  * network interface. It calls the function low_level_init() to do the
  * actual setup of the hardware.
  *
  * This function should be passed as a parameter to netif_add().
  *
  * @param netif the lwip network interface structure for this ethernetif
  * @return ERR_OK if the loopif is initialized
  *         ERR_MEM if private data couldn't be allocated
  *         any other err_t on error
  */
err_t stm32_eth_netif_init(struct netif *netif) {
  LWIP_ASSERT("netif != NULL", (netif != NULL));
  LWIP_ASSERT("netif->state != NULL", (netif->state != NULL));

  const struct mgos_eth_opts *opts = (struct mgos_eth_opts *) netif->state;
  netif->state = NULL;

  if (opts->mtu > (int) ETH_RX_BUF_SIZE) {
    LOG(LL_ERROR, ("Invalid MTU, max %d", (int) ETH_RX_BUF_SIZE));
    return ERR_MEM;
  }

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "mos";
#endif /* LWIP_NETIF_HOSTNAME */

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;

  netif->output = etharp_output;
  netif->linkoutput = low_level_output;

  struct stm32_eth_netif_state *state =
      (struct stm32_eth_netif_state *) calloc(1, sizeof(*state));
  if (state == NULL) return ERR_MEM;

  state->netif = netif;
  memcpy(&state->phy_opts, &opts->phy_opts, sizeof(state->phy_opts));

  state->sem_id = osSemaphoreCreate(&state->sem, 1);
  netif->state = s_state = state;

  netif->hwaddr_len = ETHARP_HWADDR_LEN;
  memcpy(netif->hwaddr, opts->mac, ETHARP_HWADDR_LEN);

  ETH_HandleTypeDef *heth = &state->heth;

  heth->Instance = ETH;
  heth->Init.MACAddr = netif->hwaddr;
  /* Start with no autoneg, to avoid long delay in HAL_ETH_Init,
   * autoned be performed later by our code. */
  heth->Init.AutoNegotiation = ETH_AUTONEGOTIATION_DISABLE;
  heth->Init.Speed = ETH_SPEED_10M;
  heth->Init.DuplexMode = ETH_MODE_HALFDUPLEX;
  heth->Init.RxMode = ETH_RXINTERRUPT_MODE;
  heth->Init.ChecksumMode = ETH_CHECKSUM_BY_HARDWARE;
  heth->Init.PhyAddress = opts->phy_addr;

  /* configure ethernet peripheral (GPIOs, clocks, MAC, DMA) */
  if (HAL_ETH_Init(heth) != HAL_OK) {
    LOG(LL_ERROR, ("ETH init error"));
    return ERR_IF;
  }

  HAL_ETH_DMATxDescListInit(heth, tx_descr, (uint8_t *) state->tx_buf,
                            ETH_TXBUFNB);
  HAL_ETH_DMARxDescListInit(heth, rx_descr, (uint8_t *) state->rx_buf,
                            ETH_RXBUFNB);

  netif->mtu = opts->mtu;

  netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

  /* create the task that handles the ETH_MAC */
  const osThreadAttr_t attr = {
      .name = "EthIf",
      .priority = osPriorityRealtime,
      .stack_size = INTERFACE_THREAD_STACK_SIZE,
  };
  osThreadNew((osThreadFunc_t) stm32_eth_netif_task, netif, &attr);

  stm32_set_int_handler(ETH_IRQn, stm32_eth_int_handler);
  HAL_NVIC_SetPriority(ETH_IRQn, 9, 0);
  HAL_NVIC_EnableIRQ(ETH_IRQn);

  /* Enable MAC and DMA transmission and reception */
  if (HAL_ETH_Start(heth) != HAL_OK) {
    return ERR_IF;
  }

  return ERR_OK;
}
