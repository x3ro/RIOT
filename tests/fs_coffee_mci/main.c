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
 * @brief       Test application for in-memory Coffee FS
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
#include "cfs/cfs.h"
#include "fs/coffee-mci.h"

char *teststring1 = "somedata";
char *teststring2 = "someotherdata";

static void test_file1(void)
{
    char buf[128];
    int input_length = strlen(teststring1);

    int fd = cfs_open("file1", CFS_READ | CFS_WRITE);
    TEST_ASSERT_EQUAL_INT(0, fd);

    int bytes = cfs_write(fd, teststring1, input_length);
    TEST_ASSERT_EQUAL_INT(8, bytes);


    int pos = cfs_seek(fd, 0, CFS_SEEK_SET);
    TEST_ASSERT_EQUAL_INT(0, pos);

    bytes = cfs_read(fd, buf, input_length);
    TEST_ASSERT_EQUAL_INT(8, bytes);

    buf[input_length] = '\0';

    TEST_ASSERT_EQUAL_STRING(teststring1, (char*)buf);
    cfs_close(fd);
}

static void test_file2(void)
{
    char buf[128];
    int input_length = strlen(teststring2);

    int fd = cfs_open("file2", CFS_READ | CFS_WRITE);
    TEST_ASSERT_EQUAL_INT(0, fd);

    int bytes = cfs_write(fd, teststring2, input_length);
    TEST_ASSERT_EQUAL_INT(13, bytes);

    cfs_seek(fd, 0, CFS_SEEK_SET);
    bytes = cfs_read(fd, buf, input_length);
    TEST_ASSERT_EQUAL_INT(13, bytes);

    buf[input_length] = '\0';

    TEST_ASSERT_EQUAL_STRING(teststring2, (char*)buf);
    cfs_close(fd);
}

static void test_file1and2(void) {
    char buf[128];

    int input_length1 = strlen(teststring1);
    int input_length2 = strlen(teststring2);

    int fd1 = cfs_open("file1", CFS_READ);
    int fd2 = cfs_open("file2", CFS_READ);

    int bytes = cfs_read(fd1, buf, input_length1);
    TEST_ASSERT_EQUAL_INT(8, bytes);

    bytes = cfs_read(fd2, buf+8, input_length2);
    TEST_ASSERT_EQUAL_INT(13, bytes);

    buf[input_length1 + input_length2] = '\0';
    TEST_ASSERT_EQUAL_STRING("somedatasomeotherdata", (char*)buf);
}

Test *testsfoo(void) {
    coffee_mci_init();

    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_file1),
        new_TestFixture(test_file2),
        new_TestFixture(test_file1and2),

    };

    EMB_UNIT_TESTCALLER(base64_tests, NULL, NULL, fixtures);
    return (Test *)&base64_tests;
}

int main(void)
{
#ifdef OUTPUT
    TextUIRunner_setOutputter(OUTPUTTER);
#endif

    TESTS_START();
    TESTS_RUN(testsfoo());
    TESTS_END();

    puts("xtimer_wait()");
    xtimer_usleep(100000);

    lpm_set(LPM_POWERDOWN);
    return 0;
}
