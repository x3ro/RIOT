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
#include <lpm.h>

#define myassert(condition) do { \
        if(!(condition)) { \
            printf("Assertion failed %s in file %s line %d\n", #condition, __FILE__, __LINE__); \
            xtimer_sleep(2); \
            lpm_set(LPM_POWERDOWN); \
        } \
    } while(0);


#ifdef BOARD_NATIVE
#include "storage/flash_sim.h"
#endif

#include "storage/ftl.h"
#include "storage/osl.h"
#include "xtimer.h"

#define ITERATIONS 2000
#define REPS 10

unsigned char subpage_buffer[512];
unsigned char ecc_buffer[6];

#define FTL_SUBPAGE_SIZE 512

int write(const unsigned char *buffer, uint32_t page, uint32_t offset, uint16_t length);
int read(unsigned char *buffer, uint32_t page, uint32_t offset, uint16_t length);
int erase(uint32_t block);

ftl_device_s device = {
    .total_pages = 102400,
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
    .size = 30,
    .next_subpage = 0,
    .erased_until = 0,
    .free_until = 0
};


ftl_partition_s data_partition = {
    .device = &device,
    .base_offset = 30,
    .size = 69,
    .next_subpage = 0,
    .erased_until = 0,
    .free_until = 0
};



ftl_partition_s *partitions[] = {
    &index_partition,
    &data_partition
};



osl_s osl;


char sprint_buffer[16];

void sprint_double(char *buffer, double x, int precision) {
    long integral_part = (long) x;
    long exponent = (long) pow(10, precision);
    long decimal_part = (long) ((x - integral_part) * exponent);
    sprintf(buffer, "%ld.%ld", integral_part, decimal_part);
}

#ifdef BOARD_NATIVE


flash_sim fs;

int write(const unsigned char *buffer, uint32_t page, uint32_t offset, uint16_t length) {
    return flash_sim_ftl_write(&fs, (char*) buffer, page, offset, length);
}

int read(unsigned char *buffer, uint32_t page, uint32_t offset, uint16_t length) {
    return flash_sim_ftl_read(&fs, (char*) buffer, page, offset, length);
}

int erase(uint32_t block) {
    return flash_sim_ftl_erase(&fs, block);
}

void init_ftl(void) {
    fs.page_size = device.page_size;
    fs.block_size = device.pages_per_block * device.page_size;
    fs.storage_size = device.total_pages * device.page_size;

    int ret = flash_sim_init(&fs);
    myassert(ret == 0);

    ret = ftl_init(&device);
    myassert(ret == 0);
}

#endif /* Board Native */



#ifdef BOARD_MSBA2

int write(const unsigned char *buffer, uint32_t page, uint32_t offset, uint16_t length) {
    myassert(offset == 0);
    myassert(length == FTL_SUBPAGE_SIZE);
    int ret = MCI_write((unsigned char*) buffer, page, 1);
    return ret;
}

int read(unsigned char *buffer, uint32_t page, uint32_t offset, uint16_t length) {
    myassert(offset == 0);
    myassert(length == FTL_SUBPAGE_SIZE);
    int ret = MCI_read((unsigned char*) buffer, page, 1);
    return ret;
}

int erase(uint32_t block) {
    // unsigned long block_to_erase = block + 0;
    // int ret = MCI_ioctl(CTRL_ERASE_SECTOR, &block_to_erase);
    // return ret;

    unsigned int buff[2];
    buff[0] = block * device.pages_per_block;
    buff[1] = (block + 1) * device.pages_per_block - 1;
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

    myassert(status == 0);

    unsigned long sector_count = 0;
    MCI_ioctl(GET_SECTOR_COUNT, &sector_count);
    printf("sector_count: %lu\n", sector_count);

    unsigned short sector_size = 0;
    MCI_ioctl(GET_SECTOR_SIZE, &sector_size);
    printf("sector_size: %hu\n", sector_size);

    unsigned long block_size = 0;
    MCI_ioctl(GET_BLOCK_SIZE, &block_size);
    printf("block_size: %lu\n", block_size);

    int ret = ftl_init(&device);
    myassert(ret == 0);
}

#endif /* Board MSBA2 */

void init_osl(void) {
    device.partitions = partitions;
    int ret = osl_init(&osl, &device, &data_partition);
    myassert(ret == 0);
}

void benchmark_ftl_write(void) {
    memset(subpage_buffer, 0x1F, FTL_SUBPAGE_SIZE);

    int ret = 0;
    int page = 0;

    timex_t then;
    timex_t now;
    timex_t elapsed;

    // warmup
    for(int p=0; p<ITERATIONS; p++) {
        ret = ftl_write_raw(&index_partition, subpage_buffer, page);
        myassert(ret == 0);
        page++;
    }


    printf("write_raw = [\n");
    for(int i=0; i<REPS; i++) {
        xtimer_now_timex(&then);
        for(int p=0; p<ITERATIONS; p++) {
            ret = ftl_write_raw(&index_partition, subpage_buffer, page);
            myassert(ret == 0);
            page++;
        }
        xtimer_now_timex(&now);
        elapsed = timex_sub(now, then);
        timex_to_str(elapsed, sprint_buffer);
        printf("%s, \n", sprint_buffer);
    }
    printf("]\n");


    printf("write_no_ecc = [\n");
    uint16_t data_length = ftl_data_per_subpage(&device, false);
    for(int i=0; i<REPS; i++) {
        xtimer_now_timex(&then);
        for(int p=0; p<ITERATIONS; p++) {
            ret = ftl_write(&data_partition, subpage_buffer, data_length);
            myassert(ret == 0);
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
            ret = ftl_write_ecc(&data_partition, subpage_buffer, data_length);
            myassert(ret == 0);
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



    int ret = 0;
    int page = 0;

    timex_t then;
    timex_t now;
    timex_t elapsed;

    // warmup
    for(int p=0; p<ITERATIONS; p++) {
        ret = ftl_read_raw(&data_partition, subpage_buffer, page);
        myassert(ret == 0);
        page++;
    }

    subpageheader_s header;

    printf("read_raw = [\n");
    for(int i=0; i<REPS; i++) {
        xtimer_now_timex(&then);
        for(int p=0; p<ITERATIONS; p++) {
            ret = ftl_read_raw(&data_partition, subpage_buffer, page);
            myassert(ret == 0);
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
            ret = ftl_read(&data_partition, subpage_buffer, &header, page);
            myassert(ret == 0);
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
            ret = ftl_read(&data_partition, subpage_buffer, &header, page);
            myassert(ret == 0);
            page++;
        }
        xtimer_now_timex(&now);
        elapsed = timex_sub(now, then);
        timex_to_str(elapsed, sprint_buffer);
        printf("%s, \n", sprint_buffer);
    }
    printf("]\n");
}

void benchmark_osl_write(void) {
    int ret = 0;

    timex_t then;
    timex_t now;
    timex_t elapsed;

    osl_od od;
    ret = osl_stream(&osl, &od, "bench:stream", sizeof(uint32_t));
    myassert(ret == 0);

    printf("osl_write = [\n");
    for(int i=0; i<REPS; i++) {
        xtimer_now_timex(&then);
        for(int p=0; p<ITERATIONS; p++) {
            ret = osl_stream_append(&od, &p);
            myassert(ret == 0);
        }
        xtimer_now_timex(&now);
        elapsed = timex_sub(now, then);
        timex_to_str(elapsed, sprint_buffer);
        printf("%s, \n", sprint_buffer);
    }
    printf("]\n");
}


void benchmark_osl_read(void) {
    int ret = 0;

    timex_t then;
    timex_t now;
    timex_t elapsed;

    osl_od od;
    ret = osl_stream(&osl, &od, "bench:stream", sizeof(uint64_t));
    myassert(ret == 0);

    osl_iter iter;
    osl_iterator(&od, &iter);
    uint64_t x;
    uint64_t sum;

    // create some elements
    for(int p=0; p<1000; p++) {
        ret = osl_stream_append(&od, &p);
        myassert(ret == 0);
    }

    printf("osl_iterate = [\n");
    for(int i=0; i<REPS; i++) {
        xtimer_now_timex(&then);
        osl_iterator(&od, &iter);
        //for(int p=0; p<ITERATIONS; p++) {
            while(OSL_STREAM_NEXT(x, iter)) {
                sum += x;
            }

            // ret = osl_stream_get(&od, &x, p);
            // printf("ret=%d\n", ret);
            // myassert(ret == 0);
            //printf("x %d\n", x);
            //sum+=x;

        //  }
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
    assert(false);
    int ret = ftl_format(&index_partition);
    myassert(ret == 0);
    ret = ftl_format(&data_partition);
    myassert(ret == 0);
    printf("format complete\n");

    //benchmark_ftl_write();
    //benchmark_ftl_read();
    //
    //
    // OSL
    init_osl();
    //benchmark_osl_write();
    benchmark_osl_read();



    return 0;
}
