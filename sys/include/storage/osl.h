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


#define OSL_MAX_OPEN_OBJECTS 8
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



typedef struct osl_object {
    char name[OSL_MAX_NAME_LENGTH+1];
    osl_record_s tail;
    uint32_t num_objects;
    uint16_t object_size;
} osl_object_s;



typedef struct osl_s {
    ftl_device_s *device;

    osl_index_entry_s latest_index;

    uint16_t page_buffer_size;
    uint16_t page_buffer_cursor; /**< The first free byte in the page buffer, 0-indexed */
    unsigned char* page_buffer;

    uint32_t next_data_page;  // Next page to be written in data partition
    uint32_t next_index_page; // Next page to be written in index partition

    uint8_t open_objects;
    osl_object_s objects[OSL_MAX_OPEN_OBJECTS];
} osl_s;

/**
 * @brief An object descriptor which references a object which is
 * currently held in memory.
 */
typedef struct osl_od {
    osl_s* osl;
    int index; // Index of this object in the osl_s objects array
} osl_od;



/**
 * @brief Initializes the object storage
 * @param device An FTL device which must have been initialized
 * @return  errno
 */
int osl_init(osl_s *osl, ftl_device_s *device);

// Opens a stream, or creates it if it doesnt exist
int osl_stream(osl_s* osl, osl_od* od, char* name, size_t object_size);
int osl_stream_append(osl_od* cd, void* object);
int osl_stream_get(osl_od* cd, void* object_buffer, unsigned long index);


/**
 * Helper function to get the referenced object from an object descriptor.
 */
osl_object_s* osl_get_object(osl_od* cd);


#ifdef __cplusplus
}
#endif

#endif
/** @} */
