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


#include "embUnit.h"
#include "xtimer.h"
#include "lpm.h"
#include "storage/flash_sim.h"
#include "storage/ftl.h"

/* Functions for a flash_sim based FTL device */

flash_sim fs;
ftl_device_s device;

ftl_error_t write(const char *buffer, pageptr_t page, uint32_t offset, uint16_t length) {
    return flash_sim_ftl_write(&fs, buffer, page, offset, length);
}

ftl_error_t read(char *buffer, pageptr_t page, uint32_t offset, uint16_t length) {
    return flash_sim_ftl_read(&fs, buffer, page, offset, length);
}

ftl_error_t erase(blockptr_t block) {
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
    TEST_ASSERT_EQUAL_INT(E_SUCCESS, ret);

    ret = ftl_init(&device);
    TEST_ASSERT(device.index_partition.device != 0);
    TEST_ASSERT_EQUAL_INT(0, device.index_partition.base_offset);
    TEST_ASSERT_EQUAL_INT(8, device.index_partition.size);

    TEST_ASSERT(device.data_partition.device != 0);
    TEST_ASSERT_EQUAL_INT(8, device.data_partition.base_offset);
    TEST_ASSERT_EQUAL_INT(56, device.data_partition.size);

    TEST_ASSERT_EQUAL_INT(3, device.ecc_size);
    TEST_ASSERT_EQUAL_INT(E_FTL_SUCCESS, ret);
}

// static void test_init_orc(void) {
//     orc_error_t ret = orc_init(&device);
//     TEST_ASSERT_EQUAL_INT(E_ORC_SUCCESS, ret);
// }

Test *testsrunner(void) {
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_init_ftl),
        //new_TestFixture(test_test),
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
