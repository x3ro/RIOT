/*
 * Copyright (C) 2016 Lucas Jenß <lucas@x3ro.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       FTL configuration example
 *
 * @}
 */

#include <stdio.h>
#include <stdint.h>

#include "storage/ftl.h"

typedef struct ftl_device_s {
    /**
     * Callback which must write a data buffer of the given length to a certain offset
     * inside a page.
     */
    int (*_write)(const char *buffer,
                         uint32_t page,
                         uint32_t offset,
                         uint16_t length);

    /**
     * Callback which must read a data segment of the given length from a certain offset
     * inside a page and writes it to the given data buffer.
     */
    int (*_read)(char *buffer,
                        uint32_t page,
                        uint32_t offset,
                        uint16_t length);

    /**
     * Callback which must erase the given block.
     */
    int (*_erase)(uint32_t block);

    /**
     * Callback which must erase |length| blocks starting at "start_block".
     */
    int (*_bulk_erase)(uint32_t start_block, uint32_t length);

    uint32_t total_pages;       //!< Total amount of pages configured for the device
    uint16_t page_size;         //!< Page size configured for the device
    uint16_t subpage_size;      //!< Subpage size
    uint16_t pages_per_block;   //!< Amount of pages inside an erase segment (block)
    uint8_t ecc_size;           //!< Size of the ECC determined for device's subpage size

    unsigned char *subpage_buffer;  //!< Buffer for subpage read/write operations.
    unsigned char *ecc_buffer;      //!< Buffer for ECC calculation
} ftl_device_s;


int flash_driver_write(const char *buffer, uint32_t page, uint32_t offset, uint16_t length) {
    // TODO: Remove this and implement interface to the flash storage driver.
    buffer = buffer;
    page = page;
    offset = offset;
    length = length;
    return -1;
}

int flash_driver_read(char *buffer, uint32_t page, uint32_t offset, uint16_t length) {
    // TODO: Remove this and implement interface to the flash storage driver.
    buffer = buffer;
    page = page;
    offset = offset;
    length = length;
    return -1;
}

int flash_driver_erase(uint32_t block) {
    // TODO: Remove this and implement interface to the flash storage driver.
    block = block;
    return -1;
}

int flash_driver_bulk_erase(uint32_t block, uint32_t length) {
    // TODO: Remove this and implement interface to the flash storage driver.
    //       If bulk-erase is not supported by your flash device, you should remove
    //       this function.
    block = block;
    length = length;
    return -1;
}



unsigned char subpage_buffer[${subpage_size}];
unsigned char ecc_buffer[${ecc_size}];

ftl_device_s test_storage = {
    .total_pages = ${total_pages},
    .page_size = ${page_size},
    .subpage_size = ${subpage_size},
    .pages_per_block = ${pages_per_block},
    .ecc_size = ${ecc_size},

    ._write = flash_driver_write,
    ._read = flash_driver_read,
    ._erase = flash_driver_erase,
    ._bulk_erase = flash_driver_bulk_erase,

    ._subpage_buffer = &subpage_buffer,
    ._ecc_buffer = &ecc_buffer
};

${partitions}

int main(void)
{
    printf("You are running RIOT on a(n) %s board %d.\n", RIOT_BOARD, device.total_pages);
    printf("This board features a(n) %s MCU.\n", RIOT_MCU);

    return 0;
}
