/*
 * Copyright (C) 2015 Lucas Jenß
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     sys_storage
 * @brief
 * @{
 *
 * @brief       Flash Translation Layer (FTL)
 * @author      Lucas Jenß <lucas@x3ro.de>
 */

#ifndef _FS_FTL_H
#define _FS_FTL_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif



typedef enum {
    E_FTL_SUCCESS = 0,
    E_FTL_ERROR = 1,
    E_FTL_INSUFFICIENT_STORAGE = 2,
    E_FTL_OUT_OF_RANGE = 3,
    E_FTL_TOO_MUCH_DATA = 4,
    E_FTL_OUT_OF_MEMORY = 5,
    E_CORRUPT_PAGE = 6
} ftl_error_t;

typedef uint32_t pageptr_t;
typedef uint32_t subpageptr_t;
typedef uint16_t subpageoffset_t;
typedef uint32_t blockptr_t;

struct ftl_device_s;

typedef struct {
    struct ftl_device_s *device;
    uint32_t base_offset; // in blocks!!!
    uint32_t size;        // in blocks!!!
} ftl_partition_s;

typedef struct ftl_device_s {
    ftl_error_t (*write)(const char *buffer,
                         pageptr_t page,
                         uint32_t offset,
                         uint16_t length);

    ftl_error_t (*read)(char *buffer,
                        pageptr_t page,
                        uint32_t offset,
                        uint16_t length);

    ftl_error_t (*erase)(blockptr_t block);

    ftl_partition_s index_partition;
    ftl_partition_s data_partition;

    char *page_buffer;
    char *ecc_buffer;

    uint32_t total_pages;
    uint16_t page_size;
    uint16_t subpage_size;
    uint16_t pages_per_block;
    uint8_t ecc_size;
} ftl_device_s;


typedef struct __attribute__((__packed__)) {
    unsigned int data_length:16;
    unsigned int ecc_enabled:1;
    unsigned int reserved:1;
} subpageheader_s;


ftl_error_t ftl_init(ftl_device_s *device);

ftl_error_t ftl_erase(const ftl_partition_s *partition, blockptr_t block);

ftl_error_t ftl_read_raw(const ftl_partition_s *partition,
                     char *buffer,
                     subpageptr_t subpage);

ftl_error_t ftl_write_raw(const ftl_partition_s *partition,
                     const char *buffer,
                     subpageptr_t subpage);

ftl_error_t ftl_write(const ftl_partition_s *partition,
                      const char *buffer,
                      subpageptr_t subpage,
                      subpageoffset_t data_length);

ftl_error_t ftl_read(const ftl_partition_s *partition,
                     char *buffer,
                     subpageheader_s *header,
                     subpageptr_t subpage);

ftl_error_t ftl_write_ecc(const ftl_partition_s *partition,
                      const char *buffer,
                      subpageptr_t subpage,
                      subpageoffset_t data_length);

uint8_t ftl_ecc_size(const ftl_device_s *device);
subpageptr_t ftl_first_subpage_of_block(const ftl_device_s *device, blockptr_t block);
subpageoffset_t ftl_data_per_subpage(const ftl_device_s *device, bool ecc_enabled);

/**
 * @param  partition
 * @return           The number of subpages which the given partition contains
 */
uint32_t ftl_subpages_in_partition(const ftl_partition_s *partition);


#ifdef __cplusplus
}
#endif

#endif
/** @} */
