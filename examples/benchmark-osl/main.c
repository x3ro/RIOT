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

#ifdef BOARD_NATIVE
#include "storage/flash_sim.h"
#endif

#include "storage/ftl.h"
#include "storage/osl.h"
#include "xtimer.h"


#define FTL_PAGE_SIZE 512
#define FTL_SUBPAGE_SIZE 512
#define FTL_PAGES_PER_BLOCK 1024
#define FTL_TOTAL_PAGES 131072 // Using 8 MB for now


ftl_device_s device;
osl_s osl;
unsigned char page_buffer[FTL_SUBPAGE_SIZE];

char sprint_buffer[16];

// char page_buffer[FTL_SUBPAGE_SIZE];
// char expect_buffer[FTL_SUBPAGE_SIZE];

void sprint_double(char *buffer, double x, int precision) {
    long integral_part = (long) x;
    long exponent = (long) pow(10, precision);
    long decimal_part = (long) ((x - integral_part) * exponent);
    sprintf(buffer, "%ld.%ld", integral_part, decimal_part);
}

#ifdef BOARD_NATIVE


flash_sim fs;

int write(const char *buffer, pageptr_t page, uint32_t offset, uint16_t length) {
    return flash_sim_ftl_write(&fs, buffer, page, offset, length);
}

int read(char *buffer, pageptr_t page, uint32_t offset, uint16_t length) {
    return flash_sim_ftl_read(&fs, buffer, page, offset, length);
}

int erase(blockptr_t block) {
    return flash_sim_ftl_erase(&fs, block);
}

void init_ftl(void) {
    device.write = write;
    device.read = read;
    device.erase = erase;
    device.page_size = FTL_PAGE_SIZE;
    device.subpage_size = FTL_SUBPAGE_SIZE;
    device.pages_per_block = FTL_PAGES_PER_BLOCK;
    device.total_pages = FTL_TOTAL_PAGES;

    fs.page_size = device.page_size;
    fs.block_size = device.pages_per_block * device.page_size;
    fs.storage_size = device.total_pages * device.page_size;

    int ret = flash_sim_init(&fs);
    assert(ret == 0);

    ret = ftl_init(&device);
    assert(ret == 0);
}

#endif /* Board Native */



#ifdef BOARD_MSBA2

int write(const char *buffer, pageptr_t page, uint32_t offset, uint16_t length) {
    assert(offset == 0);
    assert(length == FTL_SUBPAGE_SIZE);
    int ret = MCI_write((unsigned char*) buffer, page, 1);
    return ret;
}

int read(char *buffer, pageptr_t page, uint32_t offset, uint16_t length) {
    assert(offset == 0);
    assert(length == FTL_SUBPAGE_SIZE);
    int ret = MCI_read((unsigned char*) buffer, page, 1);
    return ret;
}

int erase(blockptr_t block) {
    // unsigned long block_to_erase = block + 0;
    // int ret = MCI_ioctl(CTRL_ERASE_SECTOR, &block_to_erase);
    // return ret;

    unsigned int buff[2];
    buff[0] = block * FTL_PAGES_PER_BLOCK;
    buff[1] = (block + 1) * FTL_PAGES_PER_BLOCK - 1;
    int ret = MCI_ioctl(CTRL_ERASE_SECTOR, &buff);
    return ret;
}

void init_ftl(void) {
    DSTATUS status = MCI_initialize();
    if(status == STA_NOINIT) {
        printf("Could not initialize MCI interface :(\n");
    } else if(status == STA_NODISK) {
        printf("NO SDCard detected. Aborting\n");
    } else if(status == STA_PROTECT) {
        printf("SDCard is in read-only mode\n");
    }

    assert(status == 0);

    unsigned long sector_count = 0;
    MCI_ioctl(GET_SECTOR_COUNT, &sector_count);
    printf("sector_count: %lu\n", sector_count);

    unsigned short sector_size = 0;
    MCI_ioctl(GET_SECTOR_SIZE, &sector_size);
    printf("sector_size: %hu\n", sector_size);

    unsigned long block_size = 0;
    MCI_ioctl(GET_BLOCK_SIZE, &block_size);
    printf("block_size: %lu\n", block_size);

    device.write = write;
    device.read = read;
    device.erase = erase;
    device.page_size = FTL_PAGE_SIZE;
    device.subpage_size = FTL_SUBPAGE_SIZE;
    device.pages_per_block = FTL_PAGES_PER_BLOCK;
    device.total_pages = FTL_TOTAL_PAGES;

    int ret = ftl_init(&device);
    assert(ret == 0);
}

#endif /* Board MSBA2 */

void init_osl(void) {
    int ret = osl_init(&osl, &device);
    assert(ret == 0);
}

void benchmark_ftl_write(void) {
    memset(page_buffer, 0x1F, FTL_SUBPAGE_SIZE);

    #define ITERATIONS 2000
    #define REPS 10

    int ret = 0;
    int page = 0;

    timex_t then;
    timex_t now;
    timex_t elapsed;

    // warmup
    for(int p=0; p<ITERATIONS; p++) {
        ret = ftl_write_raw(&device.data_partition, (char*)page_buffer, page);
        assert(ret == 0);
        page++;
    }


    printf("write_raw = [\n");
    for(int i=0; i<REPS; i++) {
        xtimer_now_timex(&then);
        for(int p=0; p<ITERATIONS; p++) {
            ret = ftl_write_raw(&device.data_partition, (char*)page_buffer, page);
            assert(ret == 0);
            page++;
        }
        xtimer_now_timex(&now);
        elapsed = timex_sub(now, then);
        timex_to_str(elapsed, sprint_buffer);
        printf("%s, \n", sprint_buffer);
    }
    printf("]\n");


    printf("write_no_ecc = [\n");
    subpageoffset_t data_length = ftl_data_per_subpage(&device, false);
    for(int i=0; i<REPS; i++) {
        xtimer_now_timex(&then);
        for(int p=0; p<ITERATIONS; p++) {
            ret = ftl_write(&device.data_partition, (char*)page_buffer, page, data_length);
            assert(ret == 0);
            page++;
        }
        xtimer_now_timex(&now);
        elapsed = timex_sub(now, then);
        timex_to_str(elapsed, sprint_buffer);
        printf("%s, \n", sprint_buffer);
    }
    printf("]\n");

    printf("write_ecc = [\n");
    data_length = ftl_data_per_subpage(&device, true);
    for(int i=0; i<REPS; i++) {
        xtimer_now_timex(&then);
        for(int p=0; p<ITERATIONS; p++) {
            ret = ftl_write_ecc(&device.data_partition, (char*)page_buffer, page, data_length);
            assert(ret == 0);
            page++;
        }
        xtimer_now_timex(&now);
        elapsed = timex_sub(now, then);
        timex_to_str(elapsed, sprint_buffer);
        printf("%s, \n", sprint_buffer);
    }
    printf("]\n");
}

void benchmark_ftl_read(void) {

    #define ITERATIONS 2000
    #define REPS 10

    int ret = 0;
    int page = 0;

    timex_t then;
    timex_t now;
    timex_t elapsed;

    // warmup
    for(int p=0; p<ITERATIONS; p++) {
        ret = ftl_read_raw(&device.data_partition, (char*)page_buffer, page);
        assert(ret == 0);
        page++;
    }

    subpageheader_s header;

    printf("read_raw = [\n");
    for(int i=0; i<REPS; i++) {
        xtimer_now_timex(&then);
        for(int p=0; p<ITERATIONS; p++) {
            ret = ftl_read_raw(&device.data_partition, (char*)page_buffer, page);
            assert(ret == 0);
            page++;
        }
        xtimer_now_timex(&now);
        elapsed = timex_sub(now, then);
        timex_to_str(elapsed, sprint_buffer);
        printf("%s, \n", sprint_buffer);
    }
    printf("]\n");


    printf("read_no_ecc = [\n");
    for(int i=0; i<REPS; i++) {
        xtimer_now_timex(&then);
        for(int p=0; p<ITERATIONS; p++) {
            ret = ftl_read(&device.data_partition, (char*)page_buffer, &header, page);
            assert(ret == 0);
            page++;
        }
        xtimer_now_timex(&now);
        elapsed = timex_sub(now, then);
        timex_to_str(elapsed, sprint_buffer);
        printf("%s, \n", sprint_buffer);
    }
    printf("]\n");

    printf("read_ecc = [\n");
    for(int i=0; i<REPS; i++) {
        xtimer_now_timex(&then);
        for(int p=0; p<ITERATIONS; p++) {
            ret = ftl_read(&device.data_partition, (char*)page_buffer, &header, page);
            assert(ret == 0);
            page++;
        }
        xtimer_now_timex(&now);
        elapsed = timex_sub(now, then);
        timex_to_str(elapsed, sprint_buffer);
        printf("%s, \n", sprint_buffer);
    }
    printf("]\n");
}

int main(void)
{
    init_ftl();
    int ret = ftl_format(&device.index_partition);
    assert(ret == 0);
    printf("format complete\n");

    //init_osl();

    //benchmark_ftl_write();
    benchmark_ftl_read();




    return 0;
}
