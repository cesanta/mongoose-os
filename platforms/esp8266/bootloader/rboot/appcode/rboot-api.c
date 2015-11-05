//////////////////////////////////////////////////
// rBoot OTA and config API for ESP8266.
// Copyright 2015 Richard A Burton
// richardaburton@gmail.com
// See license.txt for license terms.
// OTA code based on SDK sample from Espressif.
//////////////////////////////////////////////////

#ifdef RBOOT_INTEGRATION
#include <rboot-integration.h>
#endif

#include <c_types.h>
#include <user_interface.h>
#include <espconn.h>
#include <mem.h>
#include <osapi.h>

#include "rboot-api.h"

#ifdef __cplusplus
extern "C" {
#endif

// get the rboot config
rboot_config ICACHE_FLASH_ATTR rboot_get_config() {
	rboot_config conf;
	spi_flash_read(BOOT_CONFIG_SECTOR * SECTOR_SIZE, (uint32*)&conf, sizeof(rboot_config));
	return conf;
}

// write the rboot config
// preserves the contents of the rest of the sector,
// so the rest of the sector can be used to store user data
// updates checksum automatically (if enabled)
bool ICACHE_FLASH_ATTR rboot_set_config(rboot_config *conf) {
	uint8 *buffer;
#ifdef BOOT_CONFIG_CHKSUM
	uint8 chksum;
	uint8 *ptr;
#endif
	
	buffer = (uint8*)os_malloc(SECTOR_SIZE);
	if (!buffer) {
		//os_printf("No ram!\r\n");
		return false;
	}
	
#ifdef BOOT_CONFIG_CHKSUM
	chksum = CHKSUM_INIT;
	for (ptr = (uint8*)conf; ptr < &conf->chksum; ptr++) {
		chksum ^= *ptr;
	}
	conf->chksum = chksum;
#endif
	
	spi_flash_read(BOOT_CONFIG_SECTOR * SECTOR_SIZE, (uint32*)buffer, SECTOR_SIZE);
	memcpy(buffer, conf, sizeof(rboot_config));
	spi_flash_erase_sector(BOOT_CONFIG_SECTOR);
	spi_flash_write(BOOT_CONFIG_SECTOR * SECTOR_SIZE, (uint32*)buffer, SECTOR_SIZE);
	
	os_free(buffer);
	return true;
}

// get current boot rom
uint8 ICACHE_FLASH_ATTR rboot_get_current_rom() {
	rboot_config conf;
	conf = rboot_get_config();
	return conf.current_rom;
}

// set current boot rom
bool ICACHE_FLASH_ATTR rboot_set_current_rom(uint8 rom) {
	rboot_config conf;
	conf = rboot_get_config();
	if (rom >= conf.count) return false;
	conf.current_rom = rom;
	return rboot_set_config(&conf);
}

// create the write status struct, based on supplied start address
rboot_write_status ICACHE_FLASH_ATTR rboot_write_init(uint32 start_addr) {
	rboot_write_status status = {0};
	status.start_addr = start_addr;
	status.start_sector = start_addr / SECTOR_SIZE;
	//status.max_sector_count = 200;
	//os_printf("init addr: 0x%08x\r\n", start_addr);
	
	return status;
}

// function to do the actual writing to flash
// call repeatedly with more data (max len per write is the flash sector size (4k))
bool ICACHE_FLASH_ATTR rboot_write_flash(rboot_write_status *status, uint8 *data, uint16 len) {
	
	bool ret = false;
	uint8 *buffer;
	
	if (data == NULL || len == 0) {
		return true;
	}
	
	// get a buffer
	buffer = (uint8 *)os_zalloc(len + status->extra_count);
	if (!buffer) {
		//os_printf("No ram!\r\n");
		return false;
	}

	// copy in any remaining bytes from last chunk
	os_memcpy(buffer, status->extra_bytes, status->extra_count);
	// copy in new data
	os_memcpy(buffer + status->extra_count, data, len);

	// calculate length, must be multiple of 4
	// save any remaining bytes for next go
	len += status->extra_count;
	status->extra_count = len % 4;
	len -= status->extra_count;
	os_memcpy(status->extra_bytes, buffer + len, status->extra_count);

	// check data will fit
	//if (status->start_addr + len < (status->start_sector + status->max_sector_count) * SECTOR_SIZE) {

		if (len > SECTOR_SIZE) {
			// to support larger writes we would need to erase current
			// (if not already done), next and possibly later sectors too
		} else {
			// check if the sector the write finishes in has been erased yet,
			// this is fine as long as data len < sector size
			if (status->last_sector_erased != (status->start_addr + len) / SECTOR_SIZE) {
				status->last_sector_erased = (status->start_addr + len) / SECTOR_SIZE;
				spi_flash_erase_sector(status->last_sector_erased);
			}
		}

		// write current chunk
		//os_printf("write addr: 0x%08x, len: 0x%04x\r\n", status->start_addr, len);
		if (spi_flash_write(status->start_addr, (uint32 *)buffer, len) == SPI_FLASH_RESULT_OK) {
			ret = true;
			status->start_addr += len;
		}
	//}

	os_free(buffer);
	return ret;
}

#ifdef __cplusplus
}
#endif
