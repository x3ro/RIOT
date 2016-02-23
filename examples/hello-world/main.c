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

unsigned char subpage_buffer[128];
unsigned char ecc_buffer[6];

ftl_device_s device = {
    .total_pages = 16384,
    .page_size = 512,
    .subpage_size = 128,
    .pages_per_block = 1024,
    .ecc_size = 6,
    .partition_count = 3,

    ._write = flash_driver_write,
    ._read = flash_driver_read,
    ._erase = flash_driver_erase,
    ._bulk_erase = flash_driver_bulk_erase,

    ._subpage_buffer = subpage_buffer,
    ._ecc_buffer = ecc_buffer
};


ftl_partition_s index_partition = {
    .device = &device,
    .base_offset = 0,
    .size = 3,
    .next_page = 0,
    .erased_until = 0,
    .free_until = 0
};


ftl_partition_s firmware_partition = {
    .device = &device,
    .base_offset = 3,
    .size = 2,
    .next_page = 0,
    .erased_until = 0,
    .free_until = 0
};


ftl_partition_s sensordata_partition = {
    .device = &device,
    .base_offset = 5,
    .size = 10,
    .next_page = 0,
    .erased_until = 0,
    .free_until = 0
};



ftl_partition_s *partitions[] = {
	&index_partition,
	&firmware_partition,
	&sensordata_partition
};

int main(void)
{
    device.partitions = partitions;
    ftl_init(&device);

    printf("You are running RIOT on a(n) %s board %d.\n", RIOT_BOARD, device.total_pages);
    printf("This board features a(n) %s MCU.\n", RIOT_MCU);

    return 0;
}

