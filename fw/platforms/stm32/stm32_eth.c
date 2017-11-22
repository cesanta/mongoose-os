/**
  ******************************************************************************
  * File Name          : ethernetif.c
  * Description        : This file provides code for the configuration
  *                      of the ethernetif.c MiddleWare.
  ******************************************************************************
  *
  * Copyright (c) 2017 STMicroelectronics International N.V.
  * All rights reserved.
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

#include <string.h>

#include "stm32_sdk_hal.h"
#include "lwip/opt.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/timeouts.h"
#include "netif/etharp.h"
#include "lwip/ethip6.h"
#include "ethernetif.h"

/* Within 'USER CODE' section, code will be kept by default at each generation
 */
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* Private define ------------------------------------------------------------*/

/* Network interface name */
#define IFNAME0 's'
#define IFNAME1 't'

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/* Private variables ---------------------------------------------------------*/
#if defined(__ICCARM__) /*!< IAR Compiler */
#pragma data_alignment = 4
#endif
__ALIGN_BEGIN ETH_DMADescTypeDef
    DMARxDscrTab[ETH_RXBUFNB] __ALIGN_END; /* Ethernet Rx MA Descriptor */

#if defined(__ICCARM__) /*!< IAR Compiler */
#pragma data_alignment = 4
#endif
__ALIGN_BEGIN ETH_DMADescTypeDef
    DMATxDscrTab[ETH_TXBUFNB] __ALIGN_END; /* Ethernet Tx DMA Descriptor */

#if defined(__ICCARM__) /*!< IAR Compiler */
#pragma data_alignment = 4
#endif
__ALIGN_BEGIN uint8_t
    Rx_Buff[ETH_RXBUFNB][ETH_RX_BUF_SIZE] __ALIGN_END; /* Ethernet Receive
                                                          Buffer */

#if defined(__ICCARM__) /*!< IAR Compiler */
#pragma data_alignment = 4
#endif
__ALIGN_BEGIN uint8_t
    Tx_Buff[ETH_TXBUFNB][ETH_TX_BUF_SIZE] __ALIGN_END; /* Ethernet Transmit
                                                          Buffer */

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */

/* Global Ethernet handle*/
ETH_HandleTypeDef heth;

/* USER CODE BEGIN 3 */

/* USER CODE END 3 */

/* Private functions ---------------------------------------------------------*/

void HAL_ETH_MspInit(ETH_HandleTypeDef *ethHandle) {
  GPIO_InitTypeDef GPIO_InitStruct;
  if (ethHandle->Instance == ETH) {
    /* USER CODE BEGIN ETH_MspInit 0 */

    /* USER CODE END ETH_MspInit 0 */
    /* Enable Peripheral clock */
    __HAL_RCC_ETH_CLK_ENABLE();

    /**ETH GPIO Configuration
    PC1     ------> ETH_MDC
    PA1     ------> ETH_REF_CLK
    PA2     ------> ETH_MDIO
    PA7     ------> ETH_CRS_DV
    PC4     ------> ETH_RXD0
    PC5     ------> ETH_RXD1
    PB13     ------> ETH_TXD1
    PG11     ------> ETH_TX_EN
    PG13     ------> ETH_TXD0
    */
    GPIO_InitStruct.Pin = RMII_MDC_Pin | RMII_RXD0_Pin | RMII_RXD1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_1 | RMII_MDIO_Pin | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = RMII_TXD1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(RMII_TXD1_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = RMII_TX_EN_Pin | RMII_TXD0_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    /* USER CODE BEGIN ETH_MspInit 1 */

    /* USER CODE END ETH_MspInit 1 */
  }
}

void HAL_ETH_MspDeInit(ETH_HandleTypeDef *ethHandle) {
  if (ethHandle->Instance == ETH) {
    /* USER CODE BEGIN ETH_MspDeInit 0 */

    /* USER CODE END ETH_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_ETH_CLK_DISABLE();

    /**ETH GPIO Configuration
    PC1     ------> ETH_MDC
    PA1     ------> ETH_REF_CLK
    PA2     ------> ETH_MDIO
    PA7     ------> ETH_CRS_DV
    PC4     ------> ETH_RXD0
    PC5     ------> ETH_RXD1
    PB13     ------> ETH_TXD1
    PG11     ------> ETH_TX_EN
    PG13     ------> ETH_TXD0
    */
    HAL_GPIO_DeInit(GPIOC, RMII_MDC_Pin | RMII_RXD0_Pin | RMII_RXD1_Pin);

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1 | RMII_MDIO_Pin | GPIO_PIN_7);

    HAL_GPIO_DeInit(RMII_TXD1_GPIO_Port, RMII_TXD1_Pin);

    HAL_GPIO_DeInit(GPIOG, RMII_TX_EN_Pin | RMII_TXD0_Pin);

    /* USER CODE BEGIN ETH_MspDeInit 1 */

    /* USER CODE END ETH_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/*******************************************************************************
                       LL Driver Interface ( LwIP stack --> ETH)
*******************************************************************************/
/**
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static void low_level_init(struct netif *netif) {
  uint32_t regvalue = 0;
  HAL_StatusTypeDef hal_eth_init_status;

  /* Init ETH */

  uint8_t MACAddr[6];
  heth.Instance = ETH;
  heth.Init.AutoNegotiation = ETH_AUTONEGOTIATION_ENABLE;
  heth.Init.PhyAddress = LAN8742A_PHY_ADDRESS;
  MACAddr[0] = 0x00;
  MACAddr[1] = 0x80;
  MACAddr[2] = 0xE1;
  MACAddr[3] = 0x01;
  MACAddr[4] = 0x02;
  MACAddr[5] = 0x03;
  heth.Init.MACAddr = &MACAddr[0];
  heth.Init.RxMode = ETH_RXPOLLING_MODE;
  heth.Init.ChecksumMode = ETH_CHECKSUM_BY_HARDWARE;
  heth.Init.MediaInterface = ETH_MEDIA_INTERFACE_RMII;

  /* USER CODE BEGIN MACADDRESS */

  /* USER CODE END MACADDRESS */

  hal_eth_init_status = HAL_ETH_Init(&heth);

  if (hal_eth_init_status == HAL_OK) {
    /* Set netif link flag */
    netif->flags |= NETIF_FLAG_LINK_UP;
  }

  /* Initialize Tx Descriptors list: Chain Mode */
  HAL_ETH_DMATxDescListInit(&heth, DMATxDscrTab, &Tx_Buff[0][0], ETH_TXBUFNB);

  /* Initialize Rx Descriptors list: Chain Mode  */
  HAL_ETH_DMARxDescListInit(&heth, DMARxDscrTab, &Rx_Buff[0][0], ETH_RXBUFNB);

#if LWIP_ARP || LWIP_ETHERNET
  /* set MAC hardware address length */
  netif->hwaddr_len = ETHARP_HWADDR_LEN;

  /* set MAC hardware address */
  netif->hwaddr[0] = heth.Init.MACAddr[0];
  netif->hwaddr[1] = heth.Init.MACAddr[1];
  netif->hwaddr[2] = heth.Init.MACAddr[2];
  netif->hwaddr[3] = heth.Init.MACAddr[3];
  netif->hwaddr[4] = heth.Init.MACAddr[4];
  netif->hwaddr[5] = heth.Init.MACAddr[5];

  /* maximum transfer unit */
  netif->mtu = 1500;

/* Accept broadcast address and ARP traffic */
/* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
#if LWIP_ARP
  netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
#else
  netif->flags |= NETIF_FLAG_BROADCAST;
#endif /* LWIP_ARP */

  /* Enable MAC and DMA transmission and reception */
  HAL_ETH_Start(&heth);

  /* USER CODE BEGIN PHY_PRE_CONFIG */

  /* USER CODE END PHY_PRE_CONFIG */

  /* Read Register Configuration */
  HAL_ETH_ReadPHYRegister(&heth, PHY_ISFR, &regvalue);
  regvalue |= (PHY_ISFR_INT4);

  /* Enable Interrupt on change of link status */
  HAL_ETH_WritePHYRegister(&heth, PHY_ISFR, regvalue);

  /* Read Register Configuration */
  HAL_ETH_ReadPHYRegister(&heth, PHY_ISFR, &regvalue);

/* USER CODE BEGIN PHY_POST_CONFIG */

/* USER CODE END PHY_POST_CONFIG */

#endif /* LWIP_ARP || LWIP_ETHERNET */

  /* USER CODE BEGIN LOW_LEVEL_INIT */

  /* USER CODE END LOW_LEVEL_INIT */
}

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and
 *type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become availale since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */

static err_t low_level_output(struct netif *netif, struct pbuf *p) {
  err_t errval;
  struct pbuf *q;
  uint8_t *buffer = (uint8_t *) (heth.TxDesc->Buffer1Addr);
  __IO ETH_DMADescTypeDef *DmaTxDesc;
  uint32_t framelength = 0;
  uint32_t bufferoffset = 0;
  uint32_t byteslefttocopy = 0;
  uint32_t payloadoffset = 0;
  DmaTxDesc = heth.TxDesc;
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

  /* Prepare transmit descriptors to give to DMA */
  HAL_ETH_TransmitFrame(&heth, framelength);

  errval = ERR_OK;

error:

  /* When Transmit Underflow flag is set, clear it and issue a Transmit Poll
   * Demand to resume transmission */
  if ((heth.Instance->DMASR & ETH_DMASR_TUS) != (uint32_t) RESET) {
    /* Clear TUS ETHERNET DMA flag */
    heth.Instance->DMASR = ETH_DMASR_TUS;

    /* Resume DMA transmission*/
    heth.Instance->DMATPDR = 0;
  }
  return errval;
}

/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
   */
static struct pbuf *low_level_input(struct netif *netif) {
  struct pbuf *p = NULL;
  struct pbuf *q;
  uint16_t len = 0;
  uint8_t *buffer;
  __IO ETH_DMADescTypeDef *dmarxdesc;
  uint32_t bufferoffset = 0;
  uint32_t payloadoffset = 0;
  uint32_t byteslefttocopy = 0;
  uint32_t i = 0;

  /* get received frame */
  if (HAL_ETH_GetReceivedFrame(&heth) != HAL_OK) return NULL;

  /* Obtain the size of the packet and put it into the "len" variable. */
  len = heth.RxFrameInfos.length;
  buffer = (uint8_t *) heth.RxFrameInfos.buffer;

  if (len > 0) {
    /* We allocate a pbuf chain of pbufs from the Lwip buffer pool */
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
  }

  if (p != NULL) {
    dmarxdesc = heth.RxFrameInfos.FSRxDesc;
    bufferoffset = 0;
    for (q = p; q != NULL; q = q->next) {
      byteslefttocopy = q->len;
      payloadoffset = 0;

      /* Check if the length of bytes to copy in current pbuf is bigger than Rx
       * buffer size*/
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
  dmarxdesc = heth.RxFrameInfos.FSRxDesc;
  /* Set Own bit in Rx descriptors: gives the buffers back to DMA */
  for (i = 0; i < heth.RxFrameInfos.SegCount; i++) {
    dmarxdesc->Status |= ETH_DMARXDESC_OWN;
    dmarxdesc = (ETH_DMADescTypeDef *) (dmarxdesc->Buffer2NextDescAddr);
  }

  /* Clear Segment_Count */
  heth.RxFrameInfos.SegCount = 0;

  /* When Rx Buffer unavailable flag is set: clear it and resume reception */
  if ((heth.Instance->DMASR & ETH_DMASR_RBUS) != (uint32_t) RESET) {
    /* Clear RBUS ETHERNET DMA flag */
    heth.Instance->DMASR = ETH_DMASR_RBUS;
    /* Resume DMA reception */
    heth.Instance->DMARPDR = 0;
  }
  return p;
}

/**
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */
void ethernetif_input(struct netif *netif)

{
  err_t err;

  struct pbuf *p;

  /* move received packet into a new pbuf */
  p = low_level_input(netif);

  /* no packet could be read, silently ignore this */
  if (p == NULL) return;

  /* entry point to the LwIP stack */
  err = netif->input(p, netif);

  if (err != ERR_OK) {
    LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
    pbuf_free(p);
    p = NULL;
  }
}

#if !LWIP_ARP
/**
 * This function has to be completed by user in case of ARP OFF.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if ...
 */
static err_t low_level_output_arp_off(struct netif *netif, struct pbuf *q,
                                      ip_addr_t *ipaddr) {
  err_t errval;
  errval = ERR_OK;

  /* USER CODE BEGIN 5 */

  /* USER CODE END 5 */

  return errval;
}
#endif /* LWIP_ARP */

/**
 * Should be called at the beginning of the program to set up the
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
err_t ethernetif_init(struct netif *netif) {
  LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
/* We directly use etharp_output() here to save a function call.
 * You can instead declare your own function an call etharp_output()
 * from it if you have to do some checks before sending (e.g. if link
 * is available...) */

#if LWIP_IPV4
#if LWIP_ARP || LWIP_ETHERNET
#if LWIP_ARP
  netif->output = etharp_output;
#else
  /* The user should write ist own code in low_level_output_arp_off function */
  netif->output = low_level_output_arp_off;
#endif /* LWIP_ARP */
#endif /* LWIP_ARP || LWIP_ETHERNET */
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
  netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */

  netif->linkoutput = low_level_output;

  /* initialize the hardware */
  low_level_init(netif);

  return ERR_OK;
}

/* USER CODE BEGIN 6 */

/**
* @brief  Returns the current time in milliseconds
*         when LWIP_TIMERS == 1 and NO_SYS == 1
* @param  None
* @retval Time
*/
u32_t sys_jiffies(void) {
  return HAL_GetTick();
}

/**
* @brief  Returns the current time in milliseconds
*         when LWIP_TIMERS == 1 and NO_SYS == 1
* @param  None
* @retval Time
*/
u32_t sys_now(void) {
  return HAL_GetTick();
}

/* USER CODE END 6 */

/**
  * @brief  This function sets the netif link status.
  * @param  netif: the network interface
  * @retval None
  */

/* USER CODE BEGIN 7 */

/* USER CODE END 7 */

#if LWIP_NETIF_LINK_CALLBACK
/**
  * @brief  Link callback function, this function is called on change of link
* status
  *         to update low level driver configuration.
* @param  netif: The network interface
  * @retval None
  */
void ethernetif_update_config(struct netif *netif) {
  __IO uint32_t tickstart = 0;
  uint32_t regvalue = 0;

  if (netif_is_link_up(netif)) {
    /* Restart the auto-negotiation */
    if (heth.Init.AutoNegotiation != ETH_AUTONEGOTIATION_DISABLE) {
      /* Enable Auto-Negotiation */
      HAL_ETH_WritePHYRegister(&heth, PHY_BCR, PHY_AUTONEGOTIATION);

      /* Get tick */
      tickstart = HAL_GetTick();

      /* Wait until the auto-negotiation will be completed */
      do {
        HAL_ETH_ReadPHYRegister(&heth, PHY_BSR, &regvalue);

        /* Check for the Timeout ( 1s ) */
        if ((HAL_GetTick() - tickstart) > 1000) {
          /* In case of timeout */
          goto error;
        }
      } while (((regvalue & PHY_AUTONEGO_COMPLETE) != PHY_AUTONEGO_COMPLETE));

      /* Read the result of the auto-negotiation */
      HAL_ETH_ReadPHYRegister(&heth, PHY_SR, &regvalue);

      /* Configure the MAC with the Duplex Mode fixed by the auto-negotiation
       * process */
      if ((regvalue & PHY_DUPLEX_STATUS) != (uint32_t) RESET) {
        /* Set Ethernet duplex mode to Full-duplex following the
         * auto-negotiation */
        heth.Init.DuplexMode = ETH_MODE_FULLDUPLEX;
      } else {
        /* Set Ethernet duplex mode to Half-duplex following the
         * auto-negotiation */
        heth.Init.DuplexMode = ETH_MODE_HALFDUPLEX;
      }
      /* Configure the MAC with the speed fixed by the auto-negotiation process
       */
      if (regvalue & PHY_SPEED_STATUS) {
        /* Set Ethernet speed to 10M following the auto-negotiation */
        heth.Init.Speed = ETH_SPEED_10M;
      } else {
        /* Set Ethernet speed to 100M following the auto-negotiation */
        heth.Init.Speed = ETH_SPEED_100M;
      }
    } else /* AutoNegotiation Disable */
    {
    error:
      /* Check parameters */
      assert_param(IS_ETH_SPEED(heth.Init.Speed));
      assert_param(IS_ETH_DUPLEX_MODE(heth.Init.DuplexMode));

      /* Set MAC Speed and Duplex Mode to PHY */
      HAL_ETH_WritePHYRegister(&heth, PHY_BCR,
                               ((uint16_t)(heth.Init.DuplexMode >> 3) |
                                (uint16_t)(heth.Init.Speed >> 1)));
    }

    /* ETHERNET MAC Re-Configuration */
    HAL_ETH_ConfigMAC(&heth, (ETH_MACInitTypeDef *) NULL);

    /* Restart MAC interface */
    HAL_ETH_Start(&heth);
  } else {
    /* Stop MAC interface */
    HAL_ETH_Stop(&heth);
  }

  ethernetif_notify_conn_changed(netif);
}

/* USER CODE BEGIN 8 */
/**
  * @brief  This function notify user about link status changement.
  * @param  netif: the network interface
  * @retval None
  */
__weak void ethernetif_notify_conn_changed(struct netif *netif) {
  /* NOTE : This is function could be implemented in user file
            when the callback is needed,
  */
}
/* USER CODE END 8 */
#endif /* LWIP_NETIF_LINK_CALLBACK */

/* USER CODE BEGIN 9 */

/* USER CODE END 9 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
