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


#define OSL_MAX_OPEN_COLLECTIONS 8
#define OSL_MAX_NAME_LENGTH 31






typedef struct osl_index_page_header_s {
    uint32_t version;
    subpageptr_t first_page; /**< First page of the index entry to which this
                                  index page belongs */
} osl_index_page_s;

typedef struct osl_index_entry_s {
    subpageptr_t first_page; /**< First page of the index entry */
} osl_index_entry_s;










typedef struct osl_record_s {
    subpageptr_t subpage; /* If subpage is 0, that means that the predecessor is on the
                             same page */
    int16_t offset;
} osl_record_s;

typedef struct __attribute__((__packed__)) {
    osl_record_s predecessor;
    unsigned int length:15;
    unsigned int is_first:1;
} osl_record_header_s;



typedef struct osl_collection {
    char name[OSL_MAX_NAME_LENGTH+1];
    osl_record_s tail;
    uint32_t num_objects;
    uint16_t object_size;
} osl_collection_s;



typedef struct osl_s {
    ftl_device_s *device;

    osl_index_entry_s latest_index;

    uint16_t page_buffer_size;
    uint16_t page_buffer_cursor; /**< The first free byte in the page buffer, 0-indexed */
    unsigned char* page_buffer;

    uint8_t open_collections;
    osl_collection_s collections[OSL_MAX_OPEN_COLLECTIONS];
} osl_s;

/**
 * @brief A collection descriptor which references a collection which is
 * currently held in memory.
 */
typedef struct osl_cd {
    osl_s* osl;
    int index;
} osl_cd;



/**
 * @brief Initializes the object storage
 * @param device An FTL device which must have been initialized
 * @return  errno
 */
int osl_init(osl_s *osl, ftl_device_s *device);


/**
 * [osl_stream_new description]
 * @param  osl         [description]
 * @param  name        [description]
 * @param  item_size   Size of a single item in this stream
 * @return             [description]
 */
osl_cd osl_stream_new(osl_s* osl, char* name, size_t object_size);
osl_cd osl_stream(osl_s* osl, char* name);
int osl_stream_append(osl_cd* cd, void* object);
int osl_stream_get(osl_cd* cd, void* object_buffer, unsigned long index);


/**
 * Helper function to get the referenced collection from a collection descriptor.
 * @param  cd [description]
 * @return    [description]
 */
osl_collection_s* osl_get_collection(osl_cd* cd);


#ifdef __cplusplus
}
#endif

#endif
/** @} */
