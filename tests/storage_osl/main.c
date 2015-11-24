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

Test *testsrunner(void) {
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_init_ftl),
        new_TestFixture(test_init_osl),
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
