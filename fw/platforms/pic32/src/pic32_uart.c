/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/miot_uart.h"

#include <xc.h>

#include "pic32_uart.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include "system_config.h"
#include "system_definitions.h"

#define DEF_UART USART_ID_1

#define DEF_TX_INT_REQ INT_SOURCE_USART_1_TRANSMIT
#define DEF_RX_INT_REQ INT_SOURCE_USART_1_RECEIVE
#define DEF_ERR_INT_REQ INT_SOURCE_USART_1_ERROR

#define DEF_UART_INT_VECTOR_FOR_ISR _UART_1_VECTOR
#define DEF_UART_INT_VECTOR INT_VECTOR_UART1
#define DEF_UART_INT_PRI INT_PRIORITY_LEVEL7
#define DEF_UART_INT_SUBPRI INT_SUBPRIORITY_LEVEL0

static DRV_HANDLE def_uart;

enum miot_init_result miot_set_stdout_uart(int uart_no) {
  if (uart_no <= 0) return MIOT_INIT_OK;
  /* TODO */
  return MIOT_INIT_UART_FAILED;
}

enum miot_init_result miot_set_stderr_uart(int uart_no) {
  if (uart_no <= 0) return MIOT_INIT_OK;
  /* TODO */
  return MIOT_INIT_UART_FAILED;
}

void pic32_uart_init(void) {
  __XC_UART = 1;

  PLIB_PORTS_PinDirectionOutputSet(PORTS_ID_0, PORT_CHANNEL_D, PORTS_BIT_POS_3);

  PLIB_PORTS_RemapOutput(PORTS_ID_0, OUTPUT_FUNC_U1TX, OUTPUT_PIN_RPD3);

  /* UART's 0 is logical 1 (high), for both Rx and Tx. */
  PLIB_USART_ReceiverIdleStateLowDisable(DEF_UART);
  PLIB_USART_TransmitterIdleIsLowDisable(DEF_UART);

  def_uart = DRV_USART_Open(DEF_UART,
                            (DRV_IO_INTENT_WRITE | DRV_IO_INTENT_NONBLOCKING));

/*
 * TODO(dfrank):
 *
 * Implement UART manually instead of calling `DRV_USART_Open` above,
 * and make transmission buffered and asynchronous
 */
#if 0
  PLIB_USART_InitializeModeGeneral(
      DEF_UART, 
      false,   //-- autobaud
      false,   //-- loopback
      false,   //-- wake from sleep
      false,   //-- irda mode
      false    //-- stop when CPU is in idle mode
      );

  PLIB_USART_InitializeOperation(
      DEF_UART, 
      USART_RECEIVE_FIFO_ONE_CHAR,
      USART_TRANSMIT_FIFO_NOT_FULL, 
      USART_ENABLE_TX_RX_USED
      );

  PLIB_USART_TransmitterEnable(DEF_UART);
  /* PLIB_USART_ReceiverEnable(DEF_UART); */

  PLIB_USART_BaudRateHighEnable(DEF_UART);

  PLIB_USART_BaudRateHighSet(
      DEF_UART, 
      80000000, 
      115200
      );

  /* Config UART interrupts */
#if 0
  /*
   * TODO(dfrank): why INT_VECTOR_UART1 is not defined??
   */
  PLIB_INT_VectorPrioritySet   (INT_ID_0, DEF_UART_INT_VECTOR, DEF_UART_INT_PRI);
  PLIB_INT_VectorSubPrioritySet(INT_ID_0, DEF_UART_INT_VECTOR, DEF_UART_INT_SUBPRI);
#endif

  PLIB_INT_SourceDisable(INT_ID_0, DEF_TX_INT_REQ);
  PLIB_INT_SourceDisable(INT_ID_0, DEF_RX_INT_REQ);
  PLIB_INT_SourceDisable(INT_ID_0, DEF_ERR_INT_REQ);

  PLIB_USART_Enable(DEF_UART);
#endif
}
