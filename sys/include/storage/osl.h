/*
 * Copyright (C) 2015 Lucas Jenß
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     sys_storage
 * @{
 *
 * @brief       Object storage layer
 * @author      Lucas Jenß <lucas@x3ro.de>
 */

#ifndef _STORAGE_ORC_H
#define _STORAGE_ORC_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "storage/ftl.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct osl_index_page_header_s {
    uint32_t version;
    subpageptr_t first_page; /**< First page of the index entry to which this
                                  index page belongs */

} osl_index_page_s;

typedef struct osl_index_entry_s {
    subpageptr_t first_page; /**< First page of the index entry */
} osl_index_entry_s;

/**
 * @brief Describes a device managed by the FTL.
 */
typedef struct osl_s {
    ftl_device_s *device;

    osl_index_entry_s latest_index;
    unsigned char* page_buffer;
} osl_s;



/**
 * @brief Initializes the object storage
 * @param device An FTL device which must have been initialized
 * @return  errno
 */
int osl_init(osl_s *osl, ftl_device_s *device);




#ifdef __cplusplus
}
#endif

#endif
/** @} */
