#ifndef MG_LOCALS_INCLUDED
#define MG_LOCALS_INCLUDED

/*
 * This file is included by mongoose.h and provides the definitions
 * necessary for Mongoose core to work w/o BSD socket headers.
 */

/* Various structs and functions, such as in_addr_t are provided by LWIP. */
#include <lwip/ip_addr.h>
#include <lwip/inet.h>

/*
 * ESP LWIP is compiled w/o socket support but we need a few declarations
 * for Mongoose core (sockaddr_in and such).
 */
#if LWIP_SOCKET
#error "Did not expect LWIP_SOCKET to be enabled."
#endif

#undef LWIP_SOCKET
#define LWIP_SOCKET 1
#include <lwip/sockets.h>
#undef LWIP_SOCKET
#define LWIP_SOCKET 0

/* Implemented in libc_replacements */
long int random(void);

#endif /* MG_LOCALS_INCLUDED */
