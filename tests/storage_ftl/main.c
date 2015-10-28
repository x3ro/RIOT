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


#define FTL_PAGE_SIZE 2048
#define FTL_SUBPAGE_SIZE 512
#define FTL_PAGES_PER_BLOCK 1024
#define FTL_TOTAL_PAGES 32768

char page_buffer[FTL_SUBPAGE_SIZE];
char expect_buffer[FTL_SUBPAGE_SIZE];



/* Functions for a flash_sim based FTL device */

flash_sim fs;
ftl_device_s device;

int write(const char *buffer, pageptr_t page, uint32_t offset, uint16_t length) {
    return flash_sim_ftl_write(&fs, buffer, page, offset, length);
}

int read(char *buffer, pageptr_t page, uint32_t offset, uint16_t length) {
    return flash_sim_ftl_read(&fs, buffer, page, offset, length);
}

int erase(blockptr_t block) {
    return flash_sim_ftl_erase(&fs, block);
}


static void test_init(void) {
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
    TEST_ASSERT_EQUAL_INT(0, ret);

    ret = ftl_init(&device);
    TEST_ASSERT(device.index_partition.device != 0);
    TEST_ASSERT_EQUAL_INT(0, device.index_partition.base_offset);
    TEST_ASSERT_EQUAL_INT(2, device.index_partition.size);

    TEST_ASSERT(device.data_partition.device != 0);
    TEST_ASSERT_EQUAL_INT(2, device.data_partition.base_offset);
    TEST_ASSERT_EQUAL_INT(30, device.data_partition.size);

    TEST_ASSERT_EQUAL_INT(6, device.ecc_size);
    TEST_ASSERT_EQUAL_INT(0, ret);
}

static void test_ecc_helpers(void) {
    ftl_device_s test;
    test.subpage_size = 256;
    TEST_ASSERT_EQUAL_INT(3, ftl_ecc_size(&test));
    test.subpage_size = 512;
    TEST_ASSERT_EQUAL_INT(6, ftl_ecc_size(&test));
    test.subpage_size = 2048;
    TEST_ASSERT_EQUAL_INT(22, ftl_ecc_size(&test));
}

static void test_size_helpers(void) {
    TEST_ASSERT_EQUAL_INT(0, ftl_first_subpage_of_block(&device, 0));
    TEST_ASSERT_EQUAL_INT(4096, ftl_first_subpage_of_block(&device, 1));
    TEST_ASSERT_EQUAL_INT(172032, ftl_first_subpage_of_block(&device, 42));


    TEST_ASSERT_EQUAL_INT(8192, ftl_subpages_in_partition(&device.index_partition));
    TEST_ASSERT_EQUAL_INT(122880, ftl_subpages_in_partition(&device.data_partition));
}

static void test_write_read_raw(void) {
    int ret;

    blockptr_t block = 0;
    ret = ftl_erase(&device.data_partition, block);
    TEST_ASSERT_EQUAL_INT(0, ret);

    memset(page_buffer, 0x0, 512);
    memset(expect_buffer, 0xFF, 512);
    subpageptr_t subpage = 0;
    ret = ftl_read_raw(&device.data_partition, page_buffer, subpage);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(0, strncmp(page_buffer, expect_buffer, FTL_SUBPAGE_SIZE));

    memset(page_buffer, 0xAB, 512);
    ret = ftl_write_raw(&device.data_partition, page_buffer, subpage);
    TEST_ASSERT_EQUAL_INT(0, ret);

    memset(page_buffer, 0x00, 512);
    memset(expect_buffer, 0xAB, 512);
    ret = ftl_read_raw(&device.data_partition, page_buffer, subpage);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(0, strncmp(page_buffer, expect_buffer, FTL_SUBPAGE_SIZE));
}

static void test_read_before_write(void) {
    int ret;

    blockptr_t block = 13;
    ret = ftl_erase(&device.data_partition, block);
    TEST_ASSERT_EQUAL_INT(0, ret);

    subpageptr_t subpage = ftl_first_subpage_of_block(&device, block);

    subpageheader_s header;
    ret = ftl_read(&device.data_partition, page_buffer, &header, subpage);
    TEST_ASSERT_EQUAL_INT(-ENOENT, ret);
}

static void test_write_read(void) {
    int ret;

    blockptr_t block = 12;
    ret = ftl_erase(&device.data_partition, block);

    bool ecc_enabled = false;
    subpageoffset_t data_length = ftl_data_per_subpage(&device, ecc_enabled);
    TEST_ASSERT_EQUAL_INT(509, data_length); // 2 bytes header removed, no ECC
    memset(page_buffer, 0xAB, data_length);

    subpageptr_t subpage = ftl_first_subpage_of_block(&device, block);

    ret = ftl_write(&device.data_partition, page_buffer, subpage, 512);
    TEST_ASSERT_EQUAL_INT(-EFBIG, ret);

    ret = ftl_write(&device.data_partition, page_buffer, subpage, data_length);
    TEST_ASSERT_EQUAL_INT(0, ret);

    subpageheader_s header;
    ret = ftl_read(&device.data_partition, page_buffer, &header, subpage);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(data_length, header.data_length);
    memset(expect_buffer, 0xAB, data_length);
    TEST_ASSERT_EQUAL_INT(0, strncmp(page_buffer, expect_buffer, data_length));
}

static void test_write_read_ecc(void) {
    int ret;

    blockptr_t block = 8;
    ret = ftl_erase(&device.data_partition, block);

    bool ecc_enabled = true;
    subpageoffset_t data_length = ftl_data_per_subpage(&device, ecc_enabled);
    TEST_ASSERT_EQUAL_INT(503, data_length); // 2 bytes header + 6 ECC removed
    memset(page_buffer, 0xAB, data_length);

    subpageptr_t subpage = ftl_first_subpage_of_block(&device, block);

    ret = ftl_write_ecc(&device.data_partition, page_buffer, subpage, 512);
    TEST_ASSERT_EQUAL_INT(-EFBIG, ret);

    ret = ftl_write_ecc(&device.data_partition, page_buffer, subpage, data_length);
    TEST_ASSERT_EQUAL_INT(0, ret);

    subpageheader_s header;
    ret = ftl_read(&device.data_partition, page_buffer, &header, subpage);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(data_length, header.data_length);
    memset(expect_buffer, 0xAB, data_length);
    TEST_ASSERT_EQUAL_INT(0, strncmp(page_buffer, expect_buffer, data_length));


    unsigned char *u_page_buffer = (unsigned char*) page_buffer;

    // Fake a broken subpage that can be corrected
    ret = ftl_erase(&device.data_partition, block);
    memset(page_buffer, 0xAB, 512);
    memcpy(page_buffer, &header, sizeof(header));
    // The correct hamming code for the 0xAB sequence
    u_page_buffer[3] = 0xFF; u_page_buffer[4] = 0x30; u_page_buffer[5] = 0xC3;
    u_page_buffer[6] = 0xFF; u_page_buffer[7] = 0xFF; u_page_buffer[8] = 0xFF;
    // The flipped bit
    u_page_buffer[27] = 0xAA;
    ret = ftl_write_raw(&device.data_partition, page_buffer, subpage);
    TEST_ASSERT_EQUAL_INT(0, ret);

    ret = ftl_read(&device.data_partition, page_buffer, &header, subpage);
    TEST_ASSERT_EQUAL_INT(0, ret);


    // Fake a broken subpage that cannot be corrected
    ret = ftl_erase(&device.data_partition, block);
    memset(page_buffer, 0xAB, 512);
    memcpy(page_buffer, &header, sizeof(header));
    // The correct hamming code for the 0xAB sequence
    u_page_buffer[3] = 0xFF; u_page_buffer[4] = 0x30; u_page_buffer[5] = 0xC3;
    u_page_buffer[6] = 0xFF; u_page_buffer[7] = 0xFF; u_page_buffer[8] = 0xFF;
    // The flipped bit
    u_page_buffer[26] = 0xAA;
    u_page_buffer[27] = 0xAA;
    ret = ftl_write_raw(&device.data_partition, page_buffer, subpage);
    TEST_ASSERT_EQUAL_INT(0, ret);

    ret = ftl_read(&device.data_partition, page_buffer, &header, subpage);
    TEST_ASSERT_EQUAL_INT(-EBADF, ret);


    // Fake a broken header that can be corrected
    ret = ftl_erase(&device.data_partition, block);
    memset(page_buffer, 0xAB, 512);
    header.data_length -= 1;
    memcpy(page_buffer, &header, sizeof(header));
    header.data_length += 1;
    // The correct hamming code for the 0xAB sequence
    u_page_buffer[3] = 0xFF; u_page_buffer[4] = 0x30; u_page_buffer[5] = 0xC3;
    u_page_buffer[6] = 0xFF; u_page_buffer[7] = 0xFF; u_page_buffer[8] = 0xFF;
    ret = ftl_write_raw(&device.data_partition, page_buffer, subpage);
    TEST_ASSERT_EQUAL_INT(0, ret);

    ret = ftl_read(&device.data_partition, page_buffer, &header, subpage);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(503, header.data_length);


    // Fake broken header that cannot be recovered
    ret = ftl_erase(&device.data_partition, block);
    memset(page_buffer, 0xAB, data_length);
    header.data_length = 0xFF;
    memcpy(page_buffer, &header, sizeof(header));
    // The correct hamming code for the 0xAB sequence
    u_page_buffer[3] = 0xFF; u_page_buffer[4] = 0x30; u_page_buffer[5] = 0xC3;
    u_page_buffer[6] = 0xFF; u_page_buffer[7] = 0xFF; u_page_buffer[8] = 0xFF;
    ret = ftl_write_raw(&device.data_partition, page_buffer, subpage);
    TEST_ASSERT_EQUAL_INT(0, ret);

    ret = ftl_read(&device.data_partition, page_buffer, &header, subpage);
    TEST_ASSERT_EQUAL_INT(-EBADF, ret);
}

static void test_out_of_bounds(void) {
    int ret;

    ret = ftl_read_raw(&device.index_partition, page_buffer, 999999);
    TEST_ASSERT_EQUAL_INT(-EFAULT, ret);

    ret = ftl_read_raw(&device.data_partition, page_buffer, 999999);
    TEST_ASSERT_EQUAL_INT(-EFAULT, ret);

    ret = ftl_write_raw(&device.index_partition, page_buffer, 999999);
    TEST_ASSERT_EQUAL_INT(-EFAULT, ret);

    ret = ftl_write_raw(&device.data_partition, page_buffer, 999999);
    TEST_ASSERT_EQUAL_INT(-EFAULT, ret);
}

static void test_format(void) {
    int ret = ftl_format(&device.index_partition);
    TEST_ASSERT_EQUAL_INT(0, ret);

    ret = ftl_format(&device.data_partition);
    TEST_ASSERT_EQUAL_INT(0, ret);

    memset(page_buffer, 0x00, 512);
    memset(expect_buffer, 0xFF, 512);

    int index_pages = device.index_partition.size * device.pages_per_block;
    int index_subpages = index_pages * (device.page_size / device.subpage_size);
    for(int i=0; i<index_subpages; i++) {
        ret = ftl_read_raw(&device.index_partition, page_buffer, i);
        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_EQUAL_INT(0, strncmp(page_buffer, expect_buffer, FTL_SUBPAGE_SIZE));
    }

    int data_pages = device.data_partition.size * device.pages_per_block;
    int data_subpages = data_pages * (device.page_size / device.subpage_size);
    for(int i=0; i<data_subpages; i++) {
        ret = ftl_read_raw(&device.data_partition, page_buffer, i);
        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_EQUAL_INT(0, strncmp(page_buffer, expect_buffer, FTL_SUBPAGE_SIZE));
    }
}


Test *testsrunner(void) {
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_init),
        new_TestFixture(test_ecc_helpers),
        new_TestFixture(test_size_helpers),
        new_TestFixture(test_write_read_raw),
        new_TestFixture(test_read_before_write),
        new_TestFixture(test_write_read),
        new_TestFixture(test_write_read_ecc),
        new_TestFixture(test_out_of_bounds),
        new_TestFixture(test_format),
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
