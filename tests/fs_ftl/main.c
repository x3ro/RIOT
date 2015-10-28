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


#include "embUnit.h"
#include "xtimer.h"
#include "lpm.h"
#include "fs/flash_sim.h"
#include "fs/ftl.h"

// flash_sim fs;
// flash_sim_error_t ret;
//char read_buffer[];
// char write_buffer[512];
// char expect_buffer[512];

char page_buffer[FTL_SUBPAGE_SIZE];
char expect_buffer[FTL_SUBPAGE_SIZE];

// void _reset(void) {
//     memset(page_buffer, 0x0, 512);
//     memset(write_buffer, 0x0, 512);
//     memset(expect_buffer, 0x0, 512);
// }

static void test_init(void) {
    ftl_error_t ret;
    ret = ftl_init();
    TEST_ASSERT_EQUAL_INT(E_FTL_SUCCESS, ret);
}

static void test1(void) {
    ftl_error_t ret;

    blockptr_t block = 0;
    ret = ftl_erase(block);

    memset(page_buffer, 0x0, 512);
    memset(expect_buffer, 0xFF, 512);
    subpageptr_t subpage = 0;
    ret = ftl_read(&ftl_partition_data, page_buffer, subpage);
    TEST_ASSERT_EQUAL_INT(E_FTL_SUCCESS, ret);
    TEST_ASSERT_EQUAL_INT(0, strncmp(page_buffer, expect_buffer, FTL_SUBPAGE_SIZE));








    // TEST_ASSERT_EQUAL_INT(0, ret);
    //TEST_ASSERT_EQUAL_INT(0, sizeof(subpageheader_s));

    // ret = flash_sim_format(&fs);
    // TEST_ASSERT_EQUAL_INT(E_NOT_INITIALIZED, ret);

    // ret = flash_sim_read(&fs, read_buffer, 0);
    // TEST_ASSERT_EQUAL_INT(E_NOT_INITIALIZED, ret);

    // ret = flash_sim_write(&fs, write_buffer, 0);
    // TEST_ASSERT_EQUAL_INT(E_NOT_INITIALIZED, ret);
    //
    //



}


Test *testsrunner(void) {
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_init),
        new_TestFixture(test1),
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
