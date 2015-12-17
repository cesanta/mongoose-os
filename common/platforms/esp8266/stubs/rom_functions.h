#ifndef _ROM_FUNCTIONS_H_
#define _ROM_FUNCTIONS_H_

#include <inttypes.h>

void uartAttach(void);
void uart_rx_intr_handler(void *arg);
int uart_rx_one_char(uint8_t *ch);
uint8_t uart_rx_one_char_block(void);
// int uart_rx_readbuff(RcvMsgBuff * rcvmsg, uint8 *dst);
int uart_tx_one_char(char ch);
void uart_div_modify(uint32_t uart_no, uint32_t baud_div);
uint32_t uart_baudrate_detect(uint32_t uart_no, uint32_t flags);
void uart_buff_switch(uint32_t uart_no);

int SendMsg(uint8_t *msg, uint8_t size);
int send_packet(const void *packet, uint8_t size);

void _putc1(char *ch);

void ets_delay_us(uint32_t us);

uint32_t SPIRead(uint32_t addr, void *dst, uint32_t size);

void _ResetVector(uint32_t x);

/* Crypto functions are from wpa_supplicant. */
int md5_vector(uint32_t num_msgs, const uint8_t *msgs[],
               const uint32_t *msg_lens, uint8_t *digest);
int sha1_vector(uint32_t num_msgs, const uint8_t *msgs[],
                const uint32_t *msg_lens, uint8_t *digest);

struct MD5Context {
  uint32_t buf[4];
  uint32_t bits[2];
  uint8_t in[64];
};

void MD5Init(struct MD5Context *ctx);
void MD5Update(struct MD5Context *ctx, void *buf, uint32_t len);
void MD5Final(uint8_t digest[16], struct MD5Context *ctx);

#endif /* _ROM_FUNCTIONS_H_ */
