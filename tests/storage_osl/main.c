/*
 * Copyright (C) Lucas Jenß
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup tests
 * @{
 *
 * @file
 * @brief       Test application for flash memory simulation
 *
 * @author      Lucas Jenß <lucas@x3ro.de>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include "embUnit.h"
#include "xtimer.h"
#include "lpm.h"
#include "storage/flash_sim.h"
#include "storage/ftl.h"
#include "storage/osl.h"

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
    assert(offset == 0);
    assert(length == FTL_SUBPAGE_SIZE);
    int ret = MCI_write(buffer, page, 1);
    return ret;
}

int read(unsigned char *buffer, uint32_t page, uint32_t offset, uint16_t length) {
    assert(offset == 0);
    assert(length == FTL_SUBPAGE_SIZE);
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
    .total_pages = 16384,
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



static void test_init_osl_too_early(void) {
    int ret = osl_init(&osl, &device, &data_partition);
    TEST_ASSERT_EQUAL_INT(-ENODEV, ret);
}

static void test_init_ftl(void) {
    // device.write = write;
    // device.read = read;
    // device.erase = erase;
    // device.page_size = 1024;
    // device.subpage_size = 256;
    // device.pages_per_block = 512;
    // device.total_pages = 32768;
    //
    device.partitions = partitions;

    fs.page_size = device.page_size;
    fs.block_size = device.pages_per_block * device.page_size;
    fs.storage_size = device.total_pages * device.page_size;
    int ret = flash_sim_init(&fs);
    TEST_ASSERT_EQUAL_INT(0, ret);

    ret = ftl_init(&device);
    TEST_ASSERT_EQUAL_INT(0, ret);

    ret = ftl_format(&index_partition);
    TEST_ASSERT_EQUAL_INT(0, ret);

    ret = ftl_format(&data_partition);
    TEST_ASSERT_EQUAL_INT(0, ret);
}

static void test_init_osl(void) {
    int ret = osl_init(&osl, &device, &data_partition);
    TEST_ASSERT_EQUAL_INT(0, ret);

    //TEST_ASSERT_EQUAL_INT(0, osl.latest_index.first_page);

    TEST_ASSERT_EQUAL_INT(503, osl.subpage_buffer_size);
    TEST_ASSERT_EQUAL_INT(0, osl.subpage_buffer_cursor);
    TEST_ASSERT(osl.subpage_buffer != 0);

    TEST_ASSERT(osl.read_buffer != 0);
    TEST_ASSERT_EQUAL_INT(0, osl.read_buffer_subpage);
    // TEST_ASSERT_EQUAL_INT(-1, osl.read_buffer_partition);

    TEST_ASSERT_EQUAL_INT(0, osl.open_objects);
    //TEST_ASSERT_EQUAL_INT(0, osl.next_data_subpage);
}

static void test_stream(void) {
    osl_od stream;
    int ret = osl_stream(&osl, &stream, "test:stream", sizeof(uint64_t));
    TEST_ASSERT(ret >= 0);
    TEST_ASSERT_EQUAL_INT(0, stream.osl->subpage_buffer_cursor);

    uint64_t x;
    int record_size = sizeof(osl_record_header_s) + sizeof(uint64_t);

    osl_object_s* object = osl_get_object(&stream);
    TEST_ASSERT_EQUAL_INT(0, object->num_objects);

    x = 1;
    ret = osl_stream_append(&stream, &x);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(record_size, stream.osl->subpage_buffer_cursor);
    TEST_ASSERT_EQUAL_INT(0, object->tail.offset);
    TEST_ASSERT_EQUAL_INT(0, object->tail.subpage);
    TEST_ASSERT_EQUAL_INT(1, object->num_objects);

    x = 2;
    ret = osl_stream_append(&stream, &x);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(record_size*2, stream.osl->subpage_buffer_cursor);
    TEST_ASSERT_EQUAL_INT(record_size, object->tail.offset);
    TEST_ASSERT_EQUAL_INT(0, object->tail.subpage);
    TEST_ASSERT_EQUAL_INT(2, object->num_objects);

    x = 3;
    ret = osl_stream_append(&stream, &x);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(record_size*3, stream.osl->subpage_buffer_cursor);
    TEST_ASSERT_EQUAL_INT(record_size*2, object->tail.offset);
    TEST_ASSERT_EQUAL_INT(0, object->tail.subpage);
    TEST_ASSERT_EQUAL_INT(3, object->num_objects);

    ret = osl_stream_get(&stream, &x, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, x);

    ret = osl_stream_get(&stream, &x, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(2, x);

    ret = osl_stream_get(&stream, &x, 2);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(3, x);

    ret = osl_stream_get(&stream, &x, 3);
    TEST_ASSERT_EQUAL_INT(-EFAULT, ret);
}

// TODO: Test osl_stream_new errors, filename too long and too many open objects

static void test_stream_beyond_buffer(void) {
    osl_od stream;
    int ret = osl_stream(&osl, &stream, "test:large_stream", sizeof(uint64_t));
    TEST_ASSERT(ret >= 0);

    uint64_t x;

    osl_od stream1;
    ret = osl_stream(&osl, &stream1, "test:large_stream_int", sizeof(int));
    TEST_ASSERT(ret >= 0);

    int x1;

    for(int i=0; i < 3000; i++) {
        x1 = i;
        ret = osl_stream_append(&stream1, &x1);
        TEST_ASSERT_EQUAL_INT(0, ret);
    }

    for(int i=0; i < 3000; i++) {
        x = i;
        ret = osl_stream_append(&stream, &x);
        TEST_ASSERT_EQUAL_INT(0, ret);
    }

    for(int i=0; i < 3000; i++) {
        ret = osl_stream_get(&stream, &x, i);
        TEST_ASSERT_EQUAL_INT(i, x);
    }

    for(int i=0; i < 3000; i++) {
        ret = osl_stream_get(&stream1, &x1, i);
        TEST_ASSERT_EQUAL_INT(i, x1);
    }
}

static void test_metadata_saving(void) {
    return;
}

Test *testsrunner(void) {
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_init_osl_too_early),
        new_TestFixture(test_init_ftl),
        new_TestFixture(test_init_osl),
        new_TestFixture(test_stream),
        new_TestFixture(test_stream_beyond_buffer),
        new_TestFixture(test_metadata_saving),
    };

    EMB_UNIT_TESTCALLER(tests, NULL, NULL, fixtures);
    return (Test *)&tests;
}

int main(void)
{
#ifdef OUTPUT
    TextUIRunner_setOutputter(OUTPUTTER);
#endif

    TESTS_START();
    TESTS_RUN(testsrunner());
    TESTS_END();

    puts("xtimer_wait()");
    xtimer_usleep(100000);

    lpm_set(LPM_POWERDOWN);
    return 0;
}
