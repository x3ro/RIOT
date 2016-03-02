/*
 * Copyright (C) 2014 Freie Universität Berlin
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
 * @brief       Hello World application
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      Ludwig Knüpfer <ludwig.knuepfer@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <diskio.h>
#include <math.h>

#include "xtimer.h"
#include "lpm.h"
#include "storage/flash_sim.h"
#include "storage/ftl.h"
#include "storage/osl.h"


#define HEXDUMP_BUFFER(buffer, size)    for(int i=0; i<size; i++) { \
                                    printf("%02x ", ((unsigned char*) buffer)[i]); \
                                    if((i+1)%32 == 0) { printf("\n"); } \
                                } \
                                printf("\nbuffer ^^^^^\n");

/* Functions for a flash_sim based FTL device */

#ifdef BOARD_NATIVE

flash_sim fs;

#define FTL_PAGE_SIZE 512
#define FTL_SUBPAGE_SIZE 512
#define FTL_PAGES_PER_BLOCK 1024
#define FTL_TOTAL_PAGES 32768



/* Functions for a flash_sim based FTL device */

int write(const unsigned char *buffer, uint32_t page, uint32_t offset, uint16_t length) {
    return flash_sim_ftl_write(&fs, (char*) buffer, page, offset, length);
}

int read(unsigned char *buffer, uint32_t page, uint32_t offset, uint16_t length) {
    return flash_sim_ftl_read(&fs, (char*) buffer, page, offset, length);
}

int erase(uint32_t block) {
    printf("erasing\n");
    return flash_sim_ftl_erase(&fs, block);
}

unsigned char subpage_buffer[512];
unsigned char ecc_buffer[6];

ftl_device_s device = {
    .total_pages = 32768,
    .page_size = 512,
    .subpage_size = 512,
    .pages_per_block = 1024,
    .ecc_size = 6,
    .partition_count = 2,

    ._write = write,
    ._read = read,
    ._erase = erase,
    ._bulk_erase = NULL,

    ._subpage_buffer = subpage_buffer,
    ._ecc_buffer = ecc_buffer
};


ftl_partition_s index_partition = {
    .device = &device,
    .base_offset = 0,
    .size = 4
};


ftl_partition_s data_partition = {
    .device = &device,
    .base_offset = 4,
    .size = 27
};



ftl_partition_s *partitions[] = {
    &index_partition,
    &data_partition
};

#endif /* Board Native */



#ifdef BOARD_MSBA2

#include <diskio.h>
#define FTL_PAGE_SIZE 512
#define FTL_SUBPAGE_SIZE 512
#define FTL_PAGES_PER_BLOCK 1024
#define FTL_TOTAL_PAGES 16384 // Using 8 MB for now

int write(const unsigned char *buffer, uint32_t page, uint32_t offset, uint16_t length) {
    assert__(offset == 0);
    assert__(length == FTL_SUBPAGE_SIZE);
    int ret = MCI_write(buffer, page, 1);
    return ret;
}

int read(unsigned char *buffer, uint32_t page, uint32_t offset, uint16_t length) {
    assert__(offset == 0);
    assert__(length == FTL_SUBPAGE_SIZE);
    int ret = MCI_read(buffer, page, 1);
    return ret;
}

int erase(uint32_t block) {
    //unsigned int block_to_erase = block + 0;
    unsigned int buff[2];
    buff[0] = block * FTL_PAGES_PER_BLOCK;
    buff[1] = (block + 1) * FTL_PAGES_PER_BLOCK - 1;
    int ret = MCI_ioctl(CTRL_ERASE_SECTOR, &buff);
    return ret;
}

unsigned char subpage_buffer[512];
unsigned char ecc_buffer[6];

ftl_device_s device = {
    .total_pages = 32768,
    .page_size = 512,
    .subpage_size = 512,
    .pages_per_block = 1024,
    .ecc_size = 6,
    .partition_count = 2,

    ._write = write,
    ._read = read,
    ._erase = erase,
    ._bulk_erase = NULL,

    ._subpage_buffer = subpage_buffer,
    ._ecc_buffer = ecc_buffer
};


ftl_partition_s index_partition = {
    .device = &device,
    .base_offset = 0,
    .size = 4
};


ftl_partition_s data_partition = {
    .device = &device,
    .base_offset = 4,
    .size = 27
};



ftl_partition_s *partitions[] = {
    &index_partition,
    &data_partition
};

#endif /* Board MSBA2 */


osl_s osl;

#ifdef BOARD_NATIVE
void init(void) {
    fs.page_size = device.page_size;
    fs.block_size = device.pages_per_block * device.page_size;
    fs.storage_size = device.total_pages * device.page_size;
    int ret = flash_sim_init(&fs);
    assert__(ret == 0);
}
#endif

#ifdef BOARD_MSBA2
void init(void) {
    printf("%s\n", __FUNCTION__);

    DSTATUS status = MCI_initialize();
    if(status == STA_NOINIT) {
        printf("Could not initialize MCI interface :(\n");
    } else if(status == STA_NODISK) {
        printf("NO SDCard detected. Aborting\n");
    } else if(status == STA_PROTECT) {
        printf("SDCard is in read-only mode\n");
    }

    assert__(status == 0);

    unsigned long sector_count = 0;
    MCI_ioctl(GET_SECTOR_COUNT, &sector_count);
    printf("sector_count: %lu\n", sector_count);

    unsigned short sector_size = 0;
    MCI_ioctl(GET_SECTOR_SIZE, &sector_size);
    printf("sector_size: %hu\n", sector_size);

    unsigned long block_size = 0;
    MCI_ioctl(GET_BLOCK_SIZE, &block_size);
    printf("block_size: %lu\n", block_size);
}
#endif

int main(void)
{
    device.partitions = partitions;
    int ret;
    int ch;

    init();



    ret = ftl_init(&device);
    assert__(ret == 0);
    printf("Initialized FTL\n");

    ret = ftl_read_raw(&index_partition, device._subpage_buffer, 0);
    assert__(ret == 0);
    //HEXDUMP_BUFFER(device._subpage_buffer, device.subpage_size);

    printf("Would you like to format the partitions? (y/n)\n");
    ch = getchar();
    printf("\n");
    if(ch == 121) {
        printf("Formatting\n");
        ret = ftl_format(&index_partition);
        assert__(ret == 0);
        ret = ftl_format(&data_partition);
        assert__(ret == 0);
        printf("Formatted partitions\n");
    }
    do { ch = getchar(); } while(ch != 10);

    ret = osl_init(&osl, &device, &data_partition);
    assert__(ret == 0);
    printf("Initialized OSL\n");

    osl_od stream;
    ret = osl_stream(&osl, &stream, "test:stream", sizeof(int));
    assert__(ret == 0);
    printf("Created test stream\n");

    osl_object_s *obj = osl_get_object(&stream);
    printf("size of stream %d", (int) obj->num_objects);
    printf("\n\n");

    printf("Previous content:\n\n");
    osl_iter iter;
    osl_iterator(&stream, &iter);
    char c;
    while(osl_stream_next(&iter, &c)) {
        //printf("===============> %c (%d)\n", c, c);
        printf("%c", c);
    }

    printf("\n\nAdd new content (finish with an '!'): \n\n");


    while(1) {
        int ch = getchar();
        //printf("(%d)\n", ch);
        if(ch == -1 || ch == 33) {
            break;
        }
        ret = osl_stream_append(&stream, &ch);
        if(ret != 0) {
            printf("error %d\n", ret);
        }
    }

    printf("Current size of stream: %d\n", (int) obj->num_objects);
    ret = osl_create_checkpoint(&osl);
    assert__(ret == 0);

    ret = ftl_read_raw(&index_partition, device._subpage_buffer, 0);
    assert__(ret == 0);
    //HEXDUMP_BUFFER(device._subpage_buffer, device.subpage_size);

    // osl_iterator(&stream, &iter);
    // while(osl_stream_next(&iter, &c)) {
    //     printf("%c", c);
    // }

    printf("\n\nBye!\n");

    lpm_set(LPM_POWERDOWN);
    return 0;
}
