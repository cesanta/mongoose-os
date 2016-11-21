/**
 * \file
 *
 * \brief  Atmel Crypto Auth hardware interface object
 *
 * Copyright (c) 2015 Atmel Corporation. All rights reserved.
 *
 * \atmel_crypto_device_library_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel integrated circuit.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \atmel_crypto_device_library_license_stop
 */

#include <stdlib.h>
#include <stdio.h>
#include "kit_phy.h"
#include "kit_protocol.h"
#include "basic/atca_helpers.h"

/** \defgroup hal_ Hardware abstraction layer (hal_)
 *
 * \brief
 * These methods define the hardware abstraction layer for communicating with a CryptoAuth device
 *
   @{ */



/** \brief HAL implementation of kit protocol init.  This function calls back to the physical protocol to send the bytes
 *  \param[in] iface  instance
 *  \return ATCA_STATUS
 */
ATCA_STATUS kit_init(ATCAIface iface)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t kitstatus = 0;
	char address[] = "board:device(00)\n";
	int addresssize = sizeof(address);
	char reply[KIT_RX_WRAP_SIZE + 4];
	int replysize = sizeof(reply);
	char rxdata[2];
	int rxsize = sizeof(rxdata);
	char selectaddress[KIT_MSG_SIZE];
	int selectaddresssize = sizeof(selectaddress);
	char selectaddresspre[] = "s:physical:select(";
	char selectaddresspost[] = ")\n";
	int copysize = 0;

	// Send the address bytes
	status = kit_phy_send(iface, address, addresssize);

	// Receive the reply to address "...(C0)\n"
	memset(reply, 0, replysize);
	status = kit_phy_receive(iface, reply, &replysize);
	if (status != ATCA_SUCCESS) return ATCA_GEN_FAIL;

	if (replysize == 4) {
		// Probably an error
		status = kit_parse_rsp(reply, replysize, &kitstatus, rxdata, &rxsize);
		if (status != ATCA_SUCCESS)
			return status;
		if (kitstatus != 0)
			return ATCA_NO_DEVICES;
	}
	rxsize = 2;
	memcpy(rxdata, strchr(reply, '(') + 1, rxsize);

	// Send the select address bytes
	memset(selectaddress, 0, selectaddresssize);
	copysize = sizeof(selectaddresspre);
	memcpy(&selectaddress[0], selectaddresspre, copysize);
	memcpy(&selectaddress[(copysize - 1)], rxdata, rxsize);
	copysize = (sizeof(selectaddresspre) + rxsize);
	memcpy(&selectaddress[(copysize - 1)], selectaddresspost, sizeof(selectaddresspost));
	copysize = (sizeof(selectaddresspre) + rxsize + sizeof(selectaddresspost));
	status = kit_phy_send(iface, selectaddress, copysize);

	// Receive the reply to select address "00()\n"
	memset(reply, 0, replysize);
	status = kit_phy_receive(iface, reply, &replysize);
	if (status != ATCA_SUCCESS) return ATCA_GEN_FAIL;

	return status;
}

/** \brief HAL implementation of kit protocol send.  This function calls back to the physical protocol to send the bytes
 *  \param[in] iface     instance
 *  \param[in] txdata    pointer to bytes to send
 *  \param[in] txlength  number of bytes to send
 *  \return ATCA_STATUS
 */
ATCA_STATUS kit_send(ATCAIface iface, uint8_t* txdata, int txlength)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	int nkitbuf = txlength * 2 + KIT_TX_WRAP_SIZE;
	char* pkitbuf = NULL;

	// Check the pointers
	if ((txdata == NULL))
		return ATCA_BAD_PARAM;
	// Wrap in kit protocol
	pkitbuf = malloc(nkitbuf);
	memset(pkitbuf, 0, nkitbuf);
	status = kit_wrap_cmd(&txdata[1], txlength, pkitbuf, &nkitbuf);
	if (status != ATCA_SUCCESS) {
		free(pkitbuf);
		return ATCA_GEN_FAIL;
	}
	// Send the bytes
	status = kit_phy_send(iface, pkitbuf, nkitbuf);

#ifdef KIT_DEBUG
	// Print the bytes
	printf("\nKit Write: %s", pkitbuf);
#endif

	// Free the bytes
	free(pkitbuf);

	return status;
}

/** \brief HAL implementation to receive bytes and unwrap from kit protocol.  This function calls back to the physical protocol to receive the bytes
 * \param[in]    iface   instance
 * \param[in]    rxdata  pointer to space to receive the data
 * \param[inout] rxsize  ptr to expected number of receive bytes to request
 * \return ATCA_STATUS
 */
ATCA_STATUS kit_receive(ATCAIface iface, uint8_t* rxdata, uint16_t* rxsize)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t kitstatus = 0;
	int nkitbuf = 0;
	int dataSize = 0;
	char* pkitbuf = NULL;

	// Check the pointers
	if ((rxdata == NULL) || (rxsize == NULL))
		return ATCA_BAD_PARAM;

	// Adjust the read buffer size
	dataSize = *rxsize;
	nkitbuf = dataSize * 2 + KIT_RX_WRAP_SIZE;
	pkitbuf = malloc(nkitbuf);
	memset(pkitbuf, 0, nkitbuf);

	// Receive the bytes
	status = kit_phy_receive(iface, pkitbuf, &nkitbuf);
	if (status != ATCA_SUCCESS) {
		free(pkitbuf);
		return ATCA_GEN_FAIL;
	}

#ifdef KIT_DEBUG
	// Print the bytes
	printf("Kit Read: %s\r", pkitbuf);
#endif

	// Unwrap from kit protocol
	memset(rxdata, 0, *rxsize);
	status = kit_parse_rsp(pkitbuf, nkitbuf, &kitstatus, rxdata, &dataSize);
	*rxsize = dataSize;

	// Free the bytes
	free(pkitbuf);

	return status;
}

/** \brief Call the wake for kit protocol
 * \param[in] iface  the interface object to send the bytes over
 * \return ATCA_STATUS
 */
ATCA_STATUS kit_wake(ATCAIface iface)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t kitstatus = 0;
	char wake[] = "s:w()\n";
	int wakesize = sizeof(wake);
	char reply[KIT_RX_WRAP_SIZE + 4];
	int replysize = sizeof(reply);
	char rxdata[10];
	int rxsize = sizeof(rxdata);

	// Send the bytes
	status = kit_phy_send(iface, wake, wakesize);

#ifdef KIT_DEBUG
	// Print the bytes
	printf("\nKit Write: %s", wake);
#endif

	// Receive the reply to wake "00(04...)\n"
	memset(reply, 0, replysize);
	status = kit_phy_receive(iface, reply, &replysize);
	if (status != ATCA_SUCCESS) return ATCA_GEN_FAIL;

#ifdef KIT_DEBUG
	// Print the bytes
	printf("Kit Read: %s\n", reply);
#endif

	// Unwrap from kit protocol
	memset(rxdata, 0, rxsize);
	status = kit_parse_rsp(reply, replysize, &kitstatus, rxdata, &rxsize);

	return status;
}

/** \brief Call the idle for kit protocol
 * \param[in] iface  the interface object to send the bytes over
 * \return ATCA_STATUS
 */
ATCA_STATUS kit_idle(ATCAIface iface)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t kitstatus = 0;
	char idle[] = "s:i()\n";
	int idlesize = sizeof(idle);
	char reply[KIT_RX_WRAP_SIZE];
	int replysize = sizeof(reply);
	char rxdata[10];
	int rxsize = sizeof(rxdata);

	// Send the bytes
	status = kit_phy_send(iface, idle, idlesize);

#ifdef KIT_DEBUG
	// Print the bytes
	printf("\nKit Write: %s", idle);
#endif

	// Receive the reply to sleep "00()\n"
	memset(reply, 0, replysize);
	status = kit_phy_receive(iface, reply, &replysize);
	if (status != ATCA_SUCCESS) return ATCA_GEN_FAIL;

#ifdef KIT_DEBUG
	// Print the bytes
	printf("Kit Read: %s\r", reply);
#endif

	// Unwrap from kit protocol
	memset(rxdata, 0, rxsize);
	status = kit_parse_rsp(reply, replysize, &kitstatus, rxdata, &rxsize);

	return status;
}

/** \brief Call the sleep for kit protocol
 * \param[in] iface  the interface object to send the bytes over
 * \return ATCA_STATUS
 */
ATCA_STATUS kit_sleep(ATCAIface iface)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	uint8_t kitstatus = 0;
	char sleep[] = "s:s()\n";
	int sleepsize = sizeof(sleep);
	char reply[KIT_RX_WRAP_SIZE];
	int replysize = sizeof(reply);
	char rxdata[10];
	int rxsize = sizeof(rxdata);

	// Send the bytes
	status = kit_phy_send(iface, sleep, sleepsize);

#ifdef KIT_DEBUG
	// Print the bytes
	printf("\nKit Write: %s", sleep);
#endif

	// Receive the reply to sleep "00()\n"
	memset(reply, 0, replysize);
	status = kit_phy_receive(iface, reply, &replysize);
	if (status != ATCA_SUCCESS) return ATCA_GEN_FAIL;

#ifdef KIT_DEBUG
	// Print the bytes
	printf("Kit Read: %s\r", reply);
#endif

	// Unwrap from kit protocol
	memset(rxdata, 0, rxsize);
	status = kit_parse_rsp(reply, replysize, &kitstatus, rxdata, &rxsize);

	return status;
}

/** \brief Wrap binary bytes in ascii kit protocol
 * \param[in] txdata pointer to the binary data to wrap
 * \param[in] txlen length of the binary data to wrap
 * \param[out] pkitcmd pointer to binary data converted to ascii kit protocol
 * \param[inout] nkitcmd pointer to the size of the binary data converted to ascii kit protocol
 * \return ATCA_STATUS
 */
ATCA_STATUS kit_wrap_cmd(uint8_t* txdata, int txlen, char* pkitcmd, int* nkitcmd)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	char cmdpre[] = "s:t(";     // sha:talk(
	char cmdpost[] = ")\n";
//	char* pkitcmd = NULL;
	int cmdAsciiLen = txlen * 2;
	int cmdlen = txlen * 2 + sizeof(cmdpre) + sizeof(cmdpost) + 1;
	int cpylen = 0;
	int cpyindex = 0;

	// Check the variables
	if (txdata == NULL || pkitcmd == NULL || nkitcmd == NULL)
		return ATCA_BAD_PARAM;
	if (*nkitcmd > cmdlen)
		return ATCA_BAD_PARAM;

	// Wrap in kit protocol
	memset(pkitcmd, 0, *nkitcmd);

	// Copy the prefix
	cpylen = (int)strlen(cmdpre);
	memcpy(&pkitcmd[cpyindex], cmdpre, cpylen);
	cpyindex += cpylen;

	// Copy the ascii binary bytes
	status = atcab_bin2hex_(txdata, txlen, &pkitcmd[cpyindex], &cmdAsciiLen, false);
	if (status != ATCA_SUCCESS) return status;
	cpyindex += cmdAsciiLen;

	// Copy the postfix
	cpylen = (int)strlen(cmdpost);
	memcpy(&pkitcmd[cpyindex], cmdpost, cpylen);
	cpyindex += cpylen;

	*nkitcmd = cpyindex;

	return status;
}

/** \brief Parse the response ascii from the kit
 * \param[out] pkitbuf pointer to ascii kit protocol data to parse
 * \param[in] nkitbuf length of the ascii kit protocol data
 * \param[in] rxdata pointer to the binary data buffer
 * \param[in] datasize size of the pointer to the binary data buffer
 * \return ATCA_STATUS
 */
ATCA_STATUS kit_parse_rsp(char* pkitbuf, int nkitbuf, uint8_t* kitstatus, uint8_t* rxdata, int* datasize)
{
	ATCA_STATUS status = ATCA_SUCCESS;
	int statusId = 0;
	int dataId = 3;
	int binSize = 1;
	int asciiDataSize = 0;
	char* endDataPtr = 0;

	// First get the kit status
	atcab_hex2bin(&pkitbuf[statusId], 2, kitstatus, &binSize);

	// Next get the binary data bytes
	endDataPtr = strchr((char*)pkitbuf, ')');
	if (endDataPtr < (&pkitbuf[dataId])) return ATCA_GEN_FAIL;
	asciiDataSize = (int)(endDataPtr - (&pkitbuf[dataId]));
	atcab_hex2bin(&pkitbuf[dataId], asciiDataSize, rxdata, datasize);

	return ATCA_SUCCESS;
}

/** @} */
