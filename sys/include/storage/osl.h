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

#define OSL_RECORD_CACHE_SIZE 6

#define OSL_INDEX_PARTITION 0
#define OSL_DATA_PARTITION 1






typedef struct osl_index_page_header_s {
    uint32_t version;
    uint32_t first_page; /**< First page of the index entry to which this
                                  index page belongs */
} osl_index_page_s;

// typedef struct osl_index_entry_s {
//     uint32_t first_page; /**< First page of the index entry */
// } osl_index_entry_s;










typedef struct osl_record_s {
    uint32_t subpage; /* If subpage is 0, that means that the predecessor is on the
                             same page */
    int16_t offset;
} osl_record_s;


typedef struct osl_record_cache_s {
    osl_record_s record;
    int32_t index;
} osl_record_cache_s;

typedef struct __attribute__((__packed__)) {
    osl_record_s predecessor;
    unsigned int length:14;     //!< Length of the data contained in this record
    unsigned int is_first:1;    //!< Is this the first record of the log?
    unsigned int has_meta:1;    //!< Is this a log entry that contains metadata?
} osl_record_header_s;

typedef struct __attribute__((__packed__)) {
    unsigned int type:8;
} osl_record_metadata_header_s;

typedef struct __attribute__((__packed__)) {
    osl_record_s record;

} osl_record_modify_header_s;



typedef struct osl_object {
    char name[OSL_MAX_NAME_LENGTH+1];
    osl_record_s tail;
    uint32_t num_objects;
    uint16_t object_size;
} osl_object_s;



typedef struct osl_s {
    ftl_device_s *device;

    uint16_t subpage_buffer_size;
    uint16_t subpage_buffer_cursor; /**< The first free byte in the page buffer, 0-indexed */
    unsigned char* subpage_buffer;

    unsigned char* read_buffer;
    uint32_t read_buffer_subpage;
    //int8_t read_buffer_partition; // if == -1, no page is in read buffer, 0 = index, 1 = data

    //uint32_t next_data_subpage;  // Next page to be written in data partition
    //uint32_t next_metadata_subpage; // Next page to be written in index partition
    //
    ftl_partition_s *data_partition;

    uint8_t open_objects;
    osl_object_s objects[OSL_MAX_OPEN_OBJECTS];

    osl_record_cache_s record_cache[OSL_RECORD_CACHE_SIZE];
    int8_t record_cache_object;

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
int osl_init(osl_s *osl, ftl_device_s *device, ftl_partition_s* data_partition);

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
