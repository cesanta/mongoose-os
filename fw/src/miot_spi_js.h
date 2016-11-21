/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_SPI_JS_H_
#define CS_FW_SRC_MIOT_SPI_JS_H_

#if MIOT_ENABLE_JS && MIOT_ENABLE_SPI_API
struct v7;
void miot_spi_api_setup(struct v7 *v7);
#endif

#endif /* CS_FW_SRC_MIOT_SPI_JS_H_ */
