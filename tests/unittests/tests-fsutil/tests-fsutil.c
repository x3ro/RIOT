/*
 * Copyright (C) 2015 Lucas Jenß
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     unittests
 * @{
 *
 * @file
 * @brief       Tests for FS util module
 *
 * @author      Lucas Jenß <lucas@x3ro.de>
 *
 * @}
 */

#include "tests-fsutil.h"
#include "fs/util.h"

static void test_flash_read(void)
{
    fs_flash_op r;

    fs_util_calc_flash_op(&r, 1, 0, 512);
    TEST_ASSERT_EQUAL_INT(0, r.start_page);
    TEST_ASSERT_EQUAL_INT(0, r.start_offset);
    TEST_ASSERT_EQUAL_INT(1, r.pages);
    TEST_ASSERT_EQUAL_INT(1, r.last_page_offset);

    fs_util_calc_flash_op(&r, 512, 0, 512);
    TEST_ASSERT_EQUAL_INT(0, r.start_page);
    TEST_ASSERT_EQUAL_INT(0, r.start_offset);
    TEST_ASSERT_EQUAL_INT(1, r.pages);
    TEST_ASSERT_EQUAL_INT(512, r.last_page_offset);

    fs_util_calc_flash_op(&r, 513, 0, 512);
    TEST_ASSERT_EQUAL_INT(0, r.start_page);
    TEST_ASSERT_EQUAL_INT(0, r.start_offset);
    TEST_ASSERT_EQUAL_INT(2, r.pages);
    TEST_ASSERT_EQUAL_INT(1, r.last_page_offset);

    fs_util_calc_flash_op(&r, 513, 512, 512);
    TEST_ASSERT_EQUAL_INT(1, r.start_page);
    TEST_ASSERT_EQUAL_INT(0, r.start_offset);
    TEST_ASSERT_EQUAL_INT(2, r.pages);
    TEST_ASSERT_EQUAL_INT(1, r.last_page_offset);

    fs_util_calc_flash_op(&r, 26, 512, 512);
    TEST_ASSERT_EQUAL_INT(1, r.start_page);
    TEST_ASSERT_EQUAL_INT(0, r.start_offset);
    TEST_ASSERT_EQUAL_INT(1, r.pages);
    TEST_ASSERT_EQUAL_INT(26, r.last_page_offset);

    fs_util_calc_flash_op(&r, 1000, 256, 512);
    TEST_ASSERT_EQUAL_INT(0, r.start_page);
    TEST_ASSERT_EQUAL_INT(256, r.start_offset);
    TEST_ASSERT_EQUAL_INT(2, r.pages);
    TEST_ASSERT_EQUAL_INT(232, r.last_page_offset);
}

Test *tests_fsutil_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_flash_read),
    };

    EMB_UNIT_TESTCALLER(test_flash_read, NULL, NULL, fixtures);

    return (Test *)&test_flash_read;
}

void tests_fsutil(void)
{
    TESTS_RUN(tests_fsutil_tests());
}
