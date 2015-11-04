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
#include "fs/flash_sim.h"
#include "fs/ftl.h"

// flash_sim fs;
// flash_sim_error_t ret;
//char read_buffer[];
// char write_buffer[512];
// char expect_buffer[512];

#define FTL_PAGE_SIZE 2048
#define FTL_SUBPAGE_SIZE 512
#define FTL_BLOCK_SIZE 2048*1024
#define FTL_SUBPAGES_PER_PAGE (FTL_SUBPAGE_SIZE / FTL_SUBPAGE_SIZE)

char page_buffer[FTL_SUBPAGE_SIZE];
char expect_buffer[FTL_SUBPAGE_SIZE];

// void _reset(void) {
//     memset(page_buffer, 0x0, 512);
//     memset(write_buffer, 0x0, 512);
//     memset(expect_buffer, 0x0, 512);
// }



/* Functions for a flash_sim based FTL device */

flash_sim fs;
ftl_device_s device;

ftl_error_t write(const char *buffer,
                  pageptr_t page,
                  uint32_t offset,
                  uint16_t length) {

    int ret = flash_sim_write_partial(&fs, buffer, page, offset, length);
    if(ret != E_SUCCESS) {
        return E_FTL_ERROR;
    }
    return E_FTL_SUCCESS;
}

ftl_error_t read(char *buffer,
                 pageptr_t page,
                 uint32_t offset,
                 uint16_t length) {

    int ret = flash_sim_read_partial(&fs, buffer, page, offset, length);
    if(ret != E_SUCCESS) {
        return E_FTL_ERROR;
    }
    return E_FTL_SUCCESS;
}

ftl_error_t erase(blockptr_t block) {
    int ret = flash_sim_erase(&fs, block);
    if(ret != E_SUCCESS) {
        return E_FTL_ERROR;
    }
    return E_FTL_SUCCESS;
}


static void test_init(void) {
    device.write = write;
    device.read = read;
    device.erase = erase;
    device.page_size = 2048;
    device.subpage_size = 512;
    device.pages_per_block = 1024;
    device.total_pages = 32768;

    fs.page_size = device.page_size;
    fs.block_size = device.pages_per_block * device.page_size;
    fs.storage_size = device.total_pages * device.page_size;
    int ret = flash_sim_init(&fs);
    TEST_ASSERT_EQUAL_INT(E_SUCCESS, ret);

    ret = ftl_init(&device);
    TEST_ASSERT(device.index_partition.device != 0);
    TEST_ASSERT_EQUAL_INT(0, device.index_partition.base_offset);
    TEST_ASSERT_EQUAL_INT(2, device.index_partition.size);

    TEST_ASSERT(device.data_partition.device != 0);
    TEST_ASSERT_EQUAL_INT(2, device.data_partition.base_offset);
    TEST_ASSERT_EQUAL_INT(30, device.data_partition.size);

    TEST_ASSERT_EQUAL_INT(6, device.ecc_size);
    TEST_ASSERT_EQUAL_INT(E_FTL_SUCCESS, ret);
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
}

static void test_write_read_raw(void) {
    ftl_error_t ret;

    blockptr_t block = 0;
    ret = ftl_erase(&device.data_partition, block);
    TEST_ASSERT_EQUAL_INT(E_FTL_SUCCESS, ret);

    memset(page_buffer, 0x0, 512);
    memset(expect_buffer, 0xFF, 512);
    subpageptr_t subpage = 0;
    ret = ftl_read_raw(&device.data_partition, page_buffer, subpage);
    TEST_ASSERT_EQUAL_INT(E_FTL_SUCCESS, ret);
    TEST_ASSERT_EQUAL_INT(0, strncmp(page_buffer, expect_buffer, FTL_SUBPAGE_SIZE));

    memset(page_buffer, 0xAB, 512);
    ret = ftl_write_raw(&device.data_partition, page_buffer, subpage);
    TEST_ASSERT_EQUAL_INT(E_FTL_SUCCESS, ret);

    memset(page_buffer, 0x00, 512);
    memset(expect_buffer, 0xAB, 512);
    ret = ftl_read_raw(&device.data_partition, page_buffer, subpage);
    TEST_ASSERT_EQUAL_INT(E_FTL_SUCCESS, ret);
    TEST_ASSERT_EQUAL_INT(0, strncmp(page_buffer, expect_buffer, FTL_SUBPAGE_SIZE));
}

static void test_write_read(void) {
    ftl_error_t ret;

    blockptr_t block = 12;
    ret = ftl_erase(&device.data_partition, block);

    bool ecc_enabled = false;
    subpageoffset_t data_length = ftl_data_per_subpage(&device, ecc_enabled);
    TEST_ASSERT_EQUAL_INT(509, data_length); // 2 bytes header removed, no ECC
    memset(page_buffer, 0xAB, data_length);

    subpageptr_t subpage = ftl_first_subpage_of_block(&device, block);

    ret = ftl_write(&device.data_partition, page_buffer, subpage, 512);
    TEST_ASSERT_EQUAL_INT(E_FTL_TOO_MUCH_DATA, ret);

    ret = ftl_write(&device.data_partition, page_buffer, subpage, data_length);
    TEST_ASSERT_EQUAL_INT(E_FTL_SUCCESS, ret);

    subpageheader_s header;
    ret = ftl_read(&device.data_partition, page_buffer, &header, subpage);
    TEST_ASSERT_EQUAL_INT(E_FTL_SUCCESS, ret);
    TEST_ASSERT_EQUAL_INT(data_length, header.data_length);
    memset(expect_buffer, 0xAB, data_length);
    TEST_ASSERT_EQUAL_INT(0, strncmp(page_buffer, expect_buffer, data_length));
}

static void test_write_read_ecc(void) {
    ftl_error_t ret;

    blockptr_t block = 8;
    ret = ftl_erase(&device.data_partition, block);

    bool ecc_enabled = true;
    subpageoffset_t data_length = ftl_data_per_subpage(&device, ecc_enabled);
    TEST_ASSERT_EQUAL_INT(503, data_length); // 2 bytes header + 6 ECC removed
    memset(page_buffer, 0xAB, data_length);

    subpageptr_t subpage = ftl_first_subpage_of_block(&device, block);

    ret = ftl_write_ecc(&device.data_partition, page_buffer, subpage, 512);
    TEST_ASSERT_EQUAL_INT(E_FTL_TOO_MUCH_DATA, ret);

    ret = ftl_write_ecc(&device.data_partition, page_buffer, subpage, data_length);
    TEST_ASSERT_EQUAL_INT(E_FTL_SUCCESS, ret);

    subpageheader_s header;
    ret = ftl_read(&device.data_partition, page_buffer, &header, subpage);
    TEST_ASSERT_EQUAL_INT(E_FTL_SUCCESS, ret);
    TEST_ASSERT_EQUAL_INT(data_length, header.data_length);
    memset(expect_buffer, 0xAB, data_length);
    TEST_ASSERT_EQUAL_INT(0, strncmp(page_buffer, expect_buffer, data_length));


    // Fake a broken subpage that can be corrected
    ret = ftl_erase(&device.data_partition, block);
    memset(page_buffer, 0xAB, 512);
    memcpy(page_buffer, &header, sizeof(header));
    // The correct hamming code for the 0xAB sequence
    page_buffer[3] = 0xFF; page_buffer[4] = 0x30; page_buffer[5] = 0xC3;
    page_buffer[6] = 0xFF; page_buffer[7] = 0xFF; page_buffer[8] = 0xFF;
    // The flipped bit
    page_buffer[27] = 0xAA;
    ret = ftl_write_raw(&device.data_partition, page_buffer, subpage);
    TEST_ASSERT_EQUAL_INT(E_FTL_SUCCESS, ret);

    ret = ftl_read(&device.data_partition, page_buffer, &header, subpage);
    TEST_ASSERT_EQUAL_INT(E_FTL_SUCCESS, ret);


    // Fake a broken subpage that cannot be corrected
    ret = ftl_erase(&device.data_partition, block);
    memset(page_buffer, 0xAB, 512);
    memcpy(page_buffer, &header, sizeof(header));
    // The correct hamming code for the 0xAB sequence
    page_buffer[3] = 0xFF; page_buffer[4] = 0x30; page_buffer[5] = 0xC3;
    page_buffer[6] = 0xFF; page_buffer[7] = 0xFF; page_buffer[8] = 0xFF;
    // The flipped bit
    page_buffer[26] = 0xAA;
    page_buffer[27] = 0xAA;
    ret = ftl_write_raw(&device.data_partition, page_buffer, subpage);
    TEST_ASSERT_EQUAL_INT(E_FTL_SUCCESS, ret);

    ret = ftl_read(&device.data_partition, page_buffer, &header, subpage);
    TEST_ASSERT_EQUAL_INT(E_CORRUPT_PAGE, ret);


    // Fake a broken header that can be corrected
    ret = ftl_erase(&device.data_partition, block);
    memset(page_buffer, 0xAB, 512);
    header.data_length -= 1;
    memcpy(page_buffer, &header, sizeof(header));
    header.data_length += 1;
    // The correct hamming code for the 0xAB sequence
    page_buffer[3] = 0xFF; page_buffer[4] = 0x30; page_buffer[5] = 0xC3;
    page_buffer[6] = 0xFF; page_buffer[7] = 0xFF; page_buffer[8] = 0xFF;
    ret = ftl_write_raw(&device.data_partition, page_buffer, subpage);
    TEST_ASSERT_EQUAL_INT(E_FTL_SUCCESS, ret);

    ret = ftl_read(&device.data_partition, page_buffer, &header, subpage);
    TEST_ASSERT_EQUAL_INT(E_FTL_SUCCESS, ret);
    TEST_ASSERT_EQUAL_INT(503, header.data_length);


    // Fake broken header that cannot be recovered
    ret = ftl_erase(&device.data_partition, block);
    memset(page_buffer, 0xAB, data_length);
    header.data_length = 0xFF;
    memcpy(page_buffer, &header, sizeof(header));
    // The correct hamming code for the 0xAB sequence
    page_buffer[3] = 0xFF; page_buffer[4] = 0x30; page_buffer[5] = 0xC3;
    page_buffer[6] = 0xFF; page_buffer[7] = 0xFF; page_buffer[8] = 0xFF;
    ret = ftl_write_raw(&device.data_partition, page_buffer, subpage);
    TEST_ASSERT_EQUAL_INT(E_FTL_SUCCESS, ret);

    ret = ftl_read(&device.data_partition, page_buffer, &header, subpage);
    TEST_ASSERT_EQUAL_INT(E_CORRUPT_PAGE, ret);
}


Test *testsrunner(void) {
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_init),
        new_TestFixture(test_ecc_helpers),
        new_TestFixture(test_size_helpers),
        new_TestFixture(test_write_read_raw),
        new_TestFixture(test_write_read),
        new_TestFixture(test_write_read_ecc),
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
