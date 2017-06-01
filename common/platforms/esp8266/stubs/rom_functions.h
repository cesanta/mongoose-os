#ifndef CS_COMMON_PLATFORMS_ESP8266_STUBS_ROM_FUNCTIONS_H_
#define CS_COMMON_PLATFORMS_ESP8266_STUBS_ROM_FUNCTIONS_H_

#include <inttypes.h>

int uart_rx_one_char(uint8_t *ch);
uint8_t uart_rx_one_char_block();
int uart_tx_one_char(char ch);
void uart_div_modify(uint32_t uart_no, uint32_t baud_div);

int SendMsg(uint8_t *msg, uint8_t size);
int send_packet(uint8_t *packet, uint32_t size);
// recv_packet depends on global UartDev, better to avoid it.
// uint32_t recv_packet(void *packet, uint32_t len, uint8_t no_sync);

void _putc1(char *ch);

void ets_delay_us(uint32_t us);

uint32_t SPILock();
uint32_t SPIUnlock();
uint32_t SPIRead(uint32_t addr, void *dst, uint32_t size);
uint32_t SPIWrite(uint32_t addr, const uint32_t *src, uint32_t size);
uint32_t SPIEraseChip();
uint32_t SPIEraseBlock(uint32_t block_num);
uint32_t SPIEraseSector(uint32_t sector_num);
uint32_t SPI_read_status();
uint32_t Wait_SPI_Idle();
void spi_flash_attach();

void SelectSpiFunction();
void SPIFlashModeConfig(uint32_t a, uint32_t b);
void SPIReadModeCnfig(uint32_t a);
uint32_t SPIParamCfg(uint32_t deviceId, uint32_t chip_size, uint32_t block_size,
                     uint32_t sector_size, uint32_t page_size,
                     uint32_t status_mask);

void Cache_Read_Disable();

void ets_delay_us(uint32_t delay_micros);

void ets_isr_mask(uint32_t ints);
void ets_isr_unmask(uint32_t ints);
typedef void (*int_handler_t)(void *arg);
int_handler_t ets_isr_attach(uint32_t int_num, int_handler_t handler,
                             void *arg);
void ets_intr_lock();
void ets_intr_unlock();
void ets_set_user_start(void (*user_start_fn)());

uint32_t rtc_get_reset_reason();
void software_reset();
void rom_phy_reset_req();

void uart_rx_intr_handler(void *arg);

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

#endif /* CS_COMMON_PLATFORMS_ESP8266_STUBS_ROM_FUNCTIONS_H_ */
