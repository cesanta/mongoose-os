#ifndef _ROM_FUNCTIONS_H_
#define _ROM_FUNCTIONS_H_

#include <inttypes.h>

int uart_rx_one_char(uint8_t *ch);
uint8_t uart_rx_one_char_block();
int uart_tx_one_char(char ch);
void uart_div_modify(uint32_t uart_no, uint32_t baud_div);

int SendMsg(uint8_t *msg, uint8_t size);
int send_packet(const void *packet, uint32_t size);
// recv_packet depends on global UartDev, better to avoid it.
// uint32_t recv_packet(void *packet, uint32_t len, uint8_t no_sync);

void _putc1(char *ch);

void ets_delay_us(uint32_t us);

uint32_t SPILock();
uint32_t SPIUnlock();
uint32_t SPIRead(uint32_t addr, void *dst, uint32_t size);
uint32_t SPIWrite(uint32_t addr, const uint8_t *src, uint32_t size);
uint32_t SPIEraseBlock(uint32_t block_num);
uint32_t SPIEraseSector(uint32_t sector_num);
void spi_flash_attach();

void memset(void *addr, uint8_t c, uint32_t len);

void ets_delay_us(uint32_t delay_micros);
void ets_isr_unmask(uint32_t ints);
typedef void (*int_handler_t)(void *arg);
int_handler_t ets_isr_attach(uint32_t int_num, int_handler_t handler,
                             void *arg);
void ets_intr_lock();
void ets_intr_unlock();

void uart_rx_intr_handler(void *arg);
void *UartDev;

void _ResetVector();

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
