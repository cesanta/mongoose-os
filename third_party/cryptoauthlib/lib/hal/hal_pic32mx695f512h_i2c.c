/**
 * \file
 * \brief ATCA Hardware abstraction layer for PIC32MX695F512H I2C over plib drivers.
 *
 * This code is structured in two parts.  Part 1 is the connection of the ATCA HAL API to the physical I2C
 * implementation. Part 2 is the xxx I2C primitives to set up the interface.
 *
 * Prerequisite: 
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

#include <plib.h>
#include <stdio.h>
#include <string.h>

#include "hal/atca_hal.h"
#include "hal/hal_pic32mx695f512h_i2c.h"


/**
 * \defgroup hal_ Hardware abstraction layer (hal_)
 *
 * \brief
 * These methods define the hardware abstraction layer for communicating with a CryptoAuth device
 * using I2C driver of ASF.
 *
@{ */

/**
 * \brief
 * Logical to physical bus mapping structure
 */
ATCAI2CMaster_t *i2c_hal_data[MAX_I2C_BUSES];   // map logical, 0-based bus number to index
int i2c_bus_ref_ct = 0;                         // total in-use count across buses
//twi_options_t opt_twi_master;


/****** I2C Driver implementation *******/
static bool StartTransfer(I2C_MODULE i2c_id, bool restart)
{
    I2C_STATUS status;

    // Send the Start (or Restart) signal
    if (restart)
    {
        I2CRepeatStart(i2c_id);
    }
    else
    {
        // Wait for the bus to be idle, then start the transfer
        while (!I2CBusIsIdle(i2c_id));

        if (I2CStart(i2c_id) != I2C_SUCCESS)
        {
            //DBPRINTF("Error: Bus collision during transfer Start\n");
            return FALSE;
        }
    }

    // Wait for the signal to complete
    do
    {
        status = I2CGetStatus(i2c_id);

    } while (!(status & I2C_START));

    return TRUE;
}

static bool TransmitOneByte(I2C_MODULE i2c_id, uint8_t data)
{
    // Wait for the transmitter to be ready
    while (!I2CTransmitterIsReady(i2c_id));

    // Transmit the byte
    if (I2CSendByte(i2c_id, data) == I2C_MASTER_BUS_COLLISION)
    {
        //DBPRINTF("Error: I2C Master Bus Collision\n");
        return FALSE;
    }

    // Wait for the transmission to finish
    while (!I2CTransmissionHasCompleted(i2c_id));

    return TRUE;
}

static uint8_t ReceiveOneByte(I2C_MODULE i2c_id, bool ack)
{
    uint8_t data;
    
    // Enable I2C receive
    I2CReceiverEnable(i2c_id, TRUE);
    
    // Wait until 1-byte is fully received
    while (!I2CReceivedDataIsAvailable(i2c_id));
    
    // Save the byte received
    data = I2CGetByte(i2c_id);
    
    // Perform acknowledgement sequence
    I2CAcknowledgeByte(i2c_id, ack);
    
    // Wait until acknowledgement is successfully sent 
    while (!I2CAcknowledgeHasCompleted(i2c_id));
    
    return data;
}

static void StopTransfer(I2C_MODULE i2c_id)
{
    I2C_STATUS  status;

    // Send the Stop signal
    I2CStop(i2c_id);

    // Wait for the signal to complete
    do
    {
        status = I2CGetStatus(i2c_id);

    } while (!(status & I2C_STOP));
}

void i2c_write(I2C_MODULE i2c_id, uint8_t address, uint8_t *data, int len)
{
    uint8_t i2cBuffer[len+1];
    int i;
    
    i2cBuffer[0] = address | 0x00;
    memcpy(&i2cBuffer[1], data, len);
    
    if (!StartTransfer(i2c_id, FALSE))
    {
        return;
    }
    for (i = 0; i < len+1; i++)
    {
        if (!TransmitOneByte(i2c_id, i2cBuffer[i]))
        {
            break;
        }
    }
    StopTransfer(i2c_id);
}

void i2c_read(I2C_MODULE i2c_id, uint8_t address, uint8_t *data, uint16_t len)
{
    uint16_t i;
    
    if (!StartTransfer(i2c_id, FALSE))
    {
        return;
    }
    
    if (!TransmitOneByte(i2c_id, (address | 0x01)))
    {
        return;
    }
    
    for (i = 0; i < len; i++)
    {
        if (i < len-1)  // send ACK
            data[i] = ReceiveOneByte(i2c_id, TRUE);
        else            // send NACK
            data[i] = ReceiveOneByte(i2c_id, FALSE);
    }
    
    StopTransfer(i2c_id);
}
/****************************************/


/**
 * \brief
 * This HAL implementation assumes you've included the ASF TWI libraries in your project, otherwise,
 * the HAL layer will not compile because the ASF TWI drivers are a dependency
 */

/** \brief discover i2c buses available for this hardware
 * this maintains a list of logical to physical bus mappings freeing the application
 * of the a-priori knowledge
 * \param[in] i2c_buses - an array of logical bus numbers
 * \param[in] max_buses - maximum number of buses the app wants to attempt to discover
 */
ATCA_STATUS hal_i2c_discover_buses(int i2c_buses[], int max_buses)
{
    return ATCA_UNIMPLEMENTED;
}

/** \brief discover any CryptoAuth devices on a given logical bus number
 * \param[in] busNum - logical bus number on which to look for CryptoAuth devices
 * \param[out] cfg[] - pointer to head of an array of interface config structures which get filled in by this method
 * \param[out] *found - number of devices found on this bus
 */
ATCA_STATUS hal_i2c_discover_devices(int busNum, ATCAIfaceCfg cfg[], int *found )
{
    return ATCA_UNIMPLEMENTED;
}

/**
 * \brief
 * hal_i2c_init manages requests to initialize a physical interface. It manages use counts so when an interface
 * has released the physical layer, it will disable the interface for some other use.
 * You can have multiple ATCAIFace instances using the same bus, and you can have multiple ATCAIFace instances on
 * multiple i2c buses, so hal_i2c_init manages these things and ATCAIFace is abstracted from the physical details.
 */

/**
 * \brief initialize an I2C interface using given config
 *
 * \param[in] hal - opaque ptr to HAL data
 * \param[in] cfg - interface configuration
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_i2c_init(void *hal, ATCAIfaceCfg *cfg)
{
    int bus = cfg->atcai2c.bus; // 0-based logical bus number
    int i;
    ATCAHAL_t *phal = (ATCAHAL_t*)hal;

    if (i2c_bus_ref_ct == 0)    // power up state, no i2c buses will have been used

    for (i = 0; i < MAX_I2C_BUSES; i++)
        i2c_hal_data[i] = NULL;

    i2c_bus_ref_ct++;   // total across buses

    if (bus >= 0 && bus < MAX_I2C_BUSES) 
    {
        //// if this is the first time this bus and interface has been created, do the physical work of enabling it
        if (i2c_hal_data[bus] == NULL) 
        {
            i2c_hal_data[bus] = malloc(sizeof(ATCAI2CMaster_t));
            i2c_hal_data[bus]->ref_ct = 1;  // buses are shared, this is the first instance

            switch (bus) 
            {
//            case 0:
//                i2c_hal_data[bus]->id = I2C0;
//                break;
            case 1:
                i2c_hal_data[bus]->id = I2C1;
                break;
//            case 2:
//                i2c_hal_data[bus]->id = I2C2;
//                break;
            case 3:
                i2c_hal_data[bus]->id = I2C3;
                break;
            }
            
            // Set the I2C baudrate
            I2CSetFrequency(i2c_hal_data[bus]->id, GetPeripheralClock(), cfg->atcai2c.baud);

            // Enable the I2C bus
            I2CEnable(i2c_hal_data[bus]->id, TRUE);
            
            // store this for use during the release phase
            i2c_hal_data[bus]->bus_index = bus;
        }
        else 
        {
            // otherwise, another interface already initialized the bus, so this interface will share it and any different
            // cfg parameters will be ignored...first one to initialize this sets the configuration
            i2c_hal_data[bus]->ref_ct++;
        }

        phal->hal_data = i2c_hal_data[bus];

        return ATCA_SUCCESS;
    }

    return ATCA_COMM_FAIL;
}

/**
 * \brief HAL implementation of I2C post init
 *
 * \param[in] iface  instance
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_i2c_post_init(ATCAIface iface)
{
    return ATCA_SUCCESS;
}

/**
 * \brief HAL implementation of I2C send over ASF
 *
 * \param[in] iface     instance
 * \param[in] txdata    pointer to space to bytes to send
 * \param[in] txlength  number of bytes to send
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_i2c_send(ATCAIface iface, uint8_t *txdata, int txlength)
{
    ATCAIfaceCfg *cfg = atgetifacecfg(iface);
    int bus = cfg->atcai2c.bus;
    
    txdata[0] = 0x03;   // insert the Word Address Value, Command token
    txlength++;         // account for word address value byte.
    
    i2c_write(i2c_hal_data[bus]->id, cfg->atcai2c.slave_address, txdata, txlength);
    
    return ATCA_SUCCESS;
}

/**
 * \brief HAL implementation of I2C receive function for ASF I2C
 *
 * \param[in] iface     instance
 * \param[in] rxdata    pointer to space to receive the data
 * \param[in] rxlength  ptr to expected number of receive bytes to request
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_i2c_receive(ATCAIface iface, uint8_t *rxdata, uint16_t *rxlength)
{
    ATCAIfaceCfg *cfg = atgetifacecfg(iface);
    int bus = cfg->atcai2c.bus;
    
    i2c_read(i2c_hal_data[bus]->id, cfg->atcai2c.slave_address, rxdata, *rxlength);
    
    return ATCA_SUCCESS;
}

/**
 * \brief method to change the bus speed of I2C
 *
 * \param[in] iface  interface on which to change bus speed
 * \param[in] speed  baud rate (typically 100000 or 400000)
 */
void change_i2c_speed(ATCAIface iface, uint32_t speed)
{
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);
	int bus = cfg->atcai2c.bus;
    
    // Disable the I2C bus
    I2CEnable(i2c_hal_data[bus]->id, FALSE);
    
    // Set the I2C baudrate
    I2CSetFrequency(i2c_hal_data[bus]->id, GetPeripheralClock(), speed);
    
    // Enable the I2C bus
    I2CEnable(i2c_hal_data[bus]->id, TRUE);
}

/**
 * \brief wake up CryptoAuth device using I2C bus
 *
 * \param[in] iface  interface to logical device to wakeup
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_i2c_wake(ATCAIface iface)
{
    ATCAIfaceCfg *cfg = atgetifacecfg(iface);
	int bus = cfg->atcai2c.bus;
	uint32_t bdrt = cfg->atcai2c.baud;
    
    uint8_t data[4], expected[4] = { 0x04, 0x11, 0x33, 0x43 };
    
    if ( bdrt != 100000 )   // if not already at 100KHz, change it
        change_i2c_speed(iface, 100000);
    
    // Send 0x00 as wake pulse
    i2c_write(i2c_hal_data[bus]->id, 0x00, NULL, NULL);
    
    atca_delay_ms(3);   // wait tWHI + tWLO which is configured based on device type and configuration structure
    //atca_delay_us(cfg->wake_delay);
    
	// if necessary, revert baud rate to what came in.
	if ( bdrt != 100000 )
        change_i2c_speed(iface, cfg->atcai2c.baud);
    
    i2c_read(i2c_hal_data[bus]->id, cfg->atcai2c.slave_address, data, 4);
    
    if (memcmp(data, expected, 4) == 0)
        return ATCA_SUCCESS;
    
    return ATCA_COMM_FAIL;
}

/**
 * \brief idle CryptoAuth device using I2C bus
 *
 * \param[in] iface  interface to logical device to idle
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_i2c_idle(ATCAIface iface)
{
    ATCAIfaceCfg *cfg = atgetifacecfg(iface);
    int bus = cfg->atcai2c.bus;
    uint8_t data[4];
    
    data[0] = 0x02; // idle word address value
    
    i2c_write(i2c_hal_data[bus]->id, cfg->atcai2c.slave_address, data, 1);
        
    return ATCA_SUCCESS;
}

/**
 * \brief sleep CryptoAuth device using I2C bus
 *
 * \param[in] iface  interface to logical device to sleep
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_i2c_sleep(ATCAIface iface)
{
    ATCAIfaceCfg *cfg = atgetifacecfg(iface);
    int bus = cfg->atcai2c.bus;
    uint8_t data[4];
    
    data[0] = 0x01; // idle word address value
    
    i2c_write(i2c_hal_data[bus]->id, cfg->atcai2c.slave_address, data, 1);
        
    return ATCA_SUCCESS;
}

/**
 * \brief manages reference count on given bus and releases resource if no more refences exist
 *
 * \param[in] hal_data - opaque pointer to hal data structure - known only to the HAL implementation
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_i2c_release(void *hal_data)
{
    ATCAI2CMaster_t *hal = (ATCAI2CMaster_t*)hal_data;
    
    i2c_bus_ref_ct--;  // track total i2c bus interface instances for consistency checking and debugging
    
    // if the use count for this bus has gone to 0 references, disable it.  protect against an unbracketed release
    if (hal && --(hal->ref_ct) <= 0 && i2c_hal_data[hal->bus_index] != NULL)
    {
        I2CEnable(hal->id, FALSE);
        free(i2c_hal_data[hal->bus_index]);
        i2c_hal_data[hal->bus_index] = NULL;
    }
    
    return ATCA_SUCCESS;
}

/** @} */