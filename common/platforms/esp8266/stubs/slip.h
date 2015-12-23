#ifndef _SLIP_H_
#define _SLIP_H_

#include <inttypes.h>

void SLIP_send(const void *pkt, uint32_t size);
uint32_t SLIP_recv(void *pkt, uint32_t max_len);

#endif /* _SLIP_H_ */
