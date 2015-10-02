/*
 * Copyright (C) 2015 Lucas Jenß
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     sys_fs
 * @{
 *
 * @file
 * @brief       MCI support for the Coffee FS
 *
 * @author      Lucas Jenß <lucas@x3ro.de>
 */

#include <stdio.h>
#include <diskio.h>
#include <string.h>

#include "cfs/cfs.h"
#include "fs/coffee-mci.h"
#include "cfs-coffee-arch.h"
#include "fs/util.h"

int coffee_mci_init(void) {
    DSTATUS status = MCI_initialize();

    if(status == STA_NOINIT) {
        printf("Could not initialize MCI interface :(\n");
        return -1;
    } else if(status == STA_NODISK) {
        printf("NO SDCard detected. Aborting\n");
        return -1;
    } else if(status == STA_PROTECT) {
        printf("SDCard is in read-only mode\n");
        return 1;
    }

    printf("success\n");
    return 0;
}

void coffee_write_mci(const char* buf, uint32_t size, cfs_offset_t offset) {
    printf("write; size: %u offset: %d\n", (unsigned int) size, offset);
    buf = buf;
    //status = MCI_write(write_buffer, op_start_sector, op_sector_count);
}


void coffee_read_mci(char* buf, uint32_t size, cfs_offset_t offset) {
    //printf("read; size: %u offset: %d\n", (unsigned int) size, offset);

    fs_flash_reading r;
    fs_util_calc_flash_reading(&r, size, offset, COFFEE_PAGE_SIZE);

    // printf("reading\n\t start_page: %lu\n\t start_offset: %lu\n\t pages_to_read: %lu\n\t last_page_length: %lu\n",
    //     (unsigned long) r.start_page,
    //     (unsigned long) r.start_offset,
    //     (unsigned long) r.pages_to_read,
    //     (unsigned long) r.last_page_length);

    unsigned char page_buffer[COFFEE_PAGE_SIZE];

    MCI_read(page_buffer, r.start_page, 1);
    memcpy(buf, page_buffer + r.start_offset, COFFEE_PAGE_SIZE - r.start_offset);

    // for(uint32_t i=1; i<(r.pages_to_read-1); i++) {
    //     MCI_read(page_buffer, r->start_page + i, 1);
    //     memcpy(buf + (i*COFFEE_PAGE_SIZE), page_buffer, COFFEE_PAGE_SIZE);
    // }

    buf = buf;
}

void coffee_erase_mci(uint32_t sector) {
    printf("erase sector %lu\n", (unsigned long) sector);
    unsigned long sect = (unsigned long) sector;
    MCI_ioctl(CTRL_ERASE_SECTOR, &sect);
}
