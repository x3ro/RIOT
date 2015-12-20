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

flash_sim fs;
ftl_device_s device;
osl_s osl;

int write(const char *buffer, pageptr_t page, uint32_t offset, uint16_t length) {
    return flash_sim_ftl_write(&fs, buffer, page, offset, length);
}

int read(char *buffer, pageptr_t page, uint32_t offset, uint16_t length) {
    return flash_sim_ftl_read(&fs, buffer, page, offset, length);
}

int erase(blockptr_t block) {
    return flash_sim_ftl_erase(&fs, block);
}


static void test_init_ftl(void) {
    device.write = write;
    device.read = read;
    device.erase = erase;
    device.page_size = 1024;
    device.subpage_size = 256;
    device.pages_per_block = 512;
    device.total_pages = 32768;

    fs.page_size = device.page_size;
    fs.block_size = device.pages_per_block * device.page_size;
    fs.storage_size = device.total_pages * device.page_size;
    int ret = flash_sim_init(&fs);
    TEST_ASSERT_EQUAL_INT(0, ret);

    ret = ftl_init(&device);
    TEST_ASSERT_EQUAL_INT(0, ret);

    ret = ftl_format(&device.index_partition);
    TEST_ASSERT_EQUAL_INT(0, ret);

    ret = ftl_format(&device.data_partition);
    TEST_ASSERT_EQUAL_INT(0, ret);
}

static void test_init_osl(void) {
    int ret = osl_init(&osl, &device);
    TEST_ASSERT_EQUAL_INT(0, ret);

    TEST_ASSERT_EQUAL_INT(0, osl.latest_index.first_page);
}

static void test_stream(void) {
    osl_cd stream = osl_stream_new(&osl, "test:stream", sizeof(uint64_t));
    TEST_ASSERT(stream.index >= 0);
    TEST_ASSERT_EQUAL_INT(0, stream.osl->page_buffer_cursor);

    int ret;
    uint64_t x;
    int record_size = sizeof(osl_record_header_s) + sizeof(uint64_t);

    osl_collection_s* collection = osl_get_collection(&stream);
    TEST_ASSERT_EQUAL_INT(0, collection->num_objects);

    x = 1;
    ret = osl_stream_append(&stream, &x);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(record_size, stream.osl->page_buffer_cursor);
    TEST_ASSERT_EQUAL_INT(0, collection->tail.offset);
    TEST_ASSERT_EQUAL_INT(0, collection->tail.subpage);
    TEST_ASSERT_EQUAL_INT(1, collection->num_objects);

    x = 2;
    ret = osl_stream_append(&stream, &x);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(record_size*2, stream.osl->page_buffer_cursor);
    TEST_ASSERT_EQUAL_INT(record_size, collection->tail.offset);
    TEST_ASSERT_EQUAL_INT(0, collection->tail.subpage);
    TEST_ASSERT_EQUAL_INT(2, collection->num_objects);

    x = 3;
    ret = osl_stream_append(&stream, &x);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(record_size*3, stream.osl->page_buffer_cursor);
    TEST_ASSERT_EQUAL_INT(record_size*2, collection->tail.offset);
    TEST_ASSERT_EQUAL_INT(0, collection->tail.subpage);
    TEST_ASSERT_EQUAL_INT(3, collection->num_objects);

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
    osl_cd stream = osl_stream_new(&osl, "test:large_stream", sizeof(uint64_t));
    TEST_ASSERT(stream.index >= 0);

    int ret;
    uint64_t x;

    for(int i=0; i < 300; i++) {
        x = i;
        ret = osl_stream_append(&stream, &x);
        TEST_ASSERT_EQUAL_INT(0, ret);
    }

    for(int i=0; i < 300; i++) {
        ret = osl_stream_append(&stream, &x);
        TEST_ASSERT_EQUAL_INT(i, x);
    }
}

Test *testsrunner(void) {
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_init_ftl),
        new_TestFixture(test_init_osl),
        new_TestFixture(test_stream),
        new_TestFixture(test_stream_beyond_buffer),
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
