/*
 * Copyright (C) 2015 Lucas Jenß
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     fs
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


#define FTL_PAGE_SIZE 2048
#define FTL_SUBPAGE_SIZE 512
#define FTL_BLOCK_SIZE 2048*1024
#define FTL_SUBPAGES_PER_PAGE (FTL_SUBPAGE_SIZE / FTL_SUBPAGE_SIZE)


typedef enum {
    E_FTL_SUCCESS = 0,
    E_FTL_ERROR = 1
} ftl_error_t;


typedef struct {
    uint32_t base_offset; // in pages!!!!
    uint32_t size;
} ftl_partition_s;

typedef struct __attribute__((__packed__)) {
    unsigned int DataLength:16;
    unsigned int ECCSize:5;
    unsigned int Flags:3;
} subpageheader_s;

typedef uint32_t subpageptr_t;
typedef uint16_t subpageoffset_t;
typedef uint32_t blockptr_t;

static const ftl_partition_s ftl_partition_data = {0, 67108864};



ftl_error_t ftl_init(void);

ftl_error_t ftl_erase(blockptr_t block);

ftl_error_t ftl_read(const ftl_partition_s *partition,
                     char *buffer,
                     subpageptr_t subpage);


#ifdef __cplusplus
}
#endif

#endif
/** @} */
