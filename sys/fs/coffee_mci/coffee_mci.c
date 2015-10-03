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
#include <assert.h>

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
    fs_flash_op op;
    fs_util_calc_flash_op(&op, size, offset, COFFEE_PAGE_SIZE);

    printf("writing\n\t start_page: %lu\n\t start_offset: %lu\n\t pages: %lu\n\t last_page_offset: %lu\n",
        (unsigned long) op.start_page,
        (unsigned long) op.start_offset,
        (unsigned long) op.pages,
        (unsigned long) op.last_page_offset);

    unsigned char page_buffer[COFFEE_PAGE_SIZE];

    assert(op.pages == 1); // TODO: Implement for more than one page.

    if(op.pages == 1) {
        MCI_read(page_buffer, op.start_page, 1);
        memcpy(page_buffer + op.start_offset, buf, size);
        MCI_write(page_buffer, op.start_page, 1);
        return;
    }
}


void coffee_read_mci(char* buf, uint32_t size, cfs_offset_t offset) {
    fs_flash_op op;
    unsigned char page_buffer[COFFEE_PAGE_SIZE];

    fs_util_calc_flash_op(&op, size, offset, COFFEE_PAGE_SIZE);

    if(op.pages < 1) {
        printf("Error: pages was < 1\n");
        printf("params size: %lu; offset: %lu\n", (unsigned long) size, (unsigned long) offset);
        printf("reading\n\t start_page: %lu\n\t start_offset: %lu\n\t pages: %lu\n\t last_page_offset: %lu\n",
            (unsigned long) op.start_page,
            (unsigned long) op.start_offset,
            (unsigned long) op.pages,
            (unsigned long) op.last_page_offset);
        return;
    }

    if(op.pages == 1) {
        MCI_read(page_buffer, op.start_page, 1);
        memcpy(buf, page_buffer + op.start_offset, op.last_page_offset);
        return;
    }

    // Handle first page
    MCI_read(page_buffer, op.start_page, 1);
    memcpy(buf, page_buffer + op.start_offset, op.page_size - op.start_offset);

    uint32_t bytes_read = op.page_size - op.start_offset;
    for(unsigned int i=1; i<(op.pages-1); i++) {
        MCI_read(page_buffer, op.start_page + i, 1);
        memcpy(buf + bytes_read, page_buffer, op.page_size);
        bytes_read += op.page_size;
    }

    // Handle last page
    MCI_read(page_buffer, op.start_page + (op.pages - 1), 1);
    memcpy(buf + bytes_read, page_buffer, op.last_page_offset);
}

void coffee_erase_mci(uint32_t sector) {
    printf("erase sector %lu\n", (unsigned long) sector);
    unsigned long sect = (unsigned long) sector;
    MCI_ioctl(CTRL_ERASE_SECTOR, &sect);
}
