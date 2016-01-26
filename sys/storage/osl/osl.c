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
 * @brief       Implementation of the object storage layer
 * @author      Lucas Jenß <lucas@x3ro.de>
 */

#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "storage/osl.h"
#include "storage/ftl.h"
#include "storage/flash_sim.h"
#include "ecc/hamming256.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#define MYDEBUG(...) DEBUG("%s: ", __FUNCTION__); \
                     DEBUG(__VA_ARGS__);


int64_t _find_first_index_page(osl_s *osl) {
    subpageheader_s header;
    int ret = ftl_read(&osl->device->index_partition, (char*)osl->page_buffer, &header, 0);

    if(ret == -ENOENT) {
        MYDEBUG("Detected empty index partition. Assuming file system is empty\n");
        return -1;
    }

    // for(int i=0; i < osl->device->subpage_size; i++) {
    //     printf("%x ", osl->page_buffer[i]);
    //     if(osl->page_buffer[i] != 0xff) {
    //         uninitialized = false;
    //         break;
    //     }
    // }

    return -1;
}

int _osl_buffer_write(osl_s* osl, osl_record_header_s* record, void* item) {
    //osl_object_s* object = osl_get_object(od);

    int record_offset = osl->page_buffer_cursor;

    if( (record_offset + sizeof(osl_record_header_s) + record->length) >= osl->page_buffer_size ) {
        MYDEBUG("Tried to write past the page buffer. Flushing to flash should come here.\n");
        return -EIO;
    }

    memcpy( osl->page_buffer + osl->page_buffer_cursor,
            record,
            sizeof(osl_record_header_s));

    osl->page_buffer_cursor += sizeof(osl_record_header_s);

    memcpy( osl->page_buffer + osl->page_buffer_cursor,
            item,
            record->length);

    osl->page_buffer_cursor += record->length;

    return record_offset;
}

int _osl_buffer_read_header(osl_s* osl, osl_record_s* record, osl_record_header_s* rh) {
    if(record->subpage != 0) {
        MYDEBUG("Tried to retrieve header not in page buffer.\n");
        return -EINVAL;
    }

    memcpy( rh,
            osl->page_buffer + record->offset,
            sizeof(osl_record_header_s));

    return 0;
}

int _osl_buffer_read_datum(osl_s* osl, osl_record_s* record, void* datum, int offset, int size) {
    if(record->subpage != 0) {
        MYDEBUG("Tried to retrieve header not in page buffer.\n");
        return -EINVAL;
    }

    memcpy( datum,
            osl->page_buffer + record->offset + sizeof(osl_record_header_s) + offset,
            size);

    return 0;

}

int _osl_record_header_get(osl_s* osl, osl_record_s* record, osl_record_header_s* rh) {
    // TODO: Implement for when record is not held in page buffer anymore
    assert(record->subpage == 0);

    // Record is still in page buffer
    if(record->subpage == 0) {
        return _osl_buffer_read_header(osl, record, rh);
    }

    return -1; // TODO
}

int _osl_buffer_flush(osl_s* osl) {
    MYDEBUG("Flushing buffer to page %d\n", osl->next_data_page);

    int ret = ftl_write_ecc(&osl->device->data_partition,
                            (char *) osl->page_buffer,
                            osl->next_data_page,
                            osl->page_buffer_size);

    if(ret != 0) {
        return ret;
    }

    memset(osl->page_buffer, 0, osl->page_buffer_size);
    osl->page_buffer_cursor = 0;
    osl->next_data_page++;
    return 0;
}


int _osl_record_datum_get(osl_s* osl, osl_record_s* record, void* datum, int offset, int size) {
    // TODO: Implement for when record is not held in page buffer anymore
    assert(record->subpage == 0);

    if(record->subpage == 0) {
        return _osl_buffer_read_datum(osl, record, datum, offset, size);
    }

    return -1; // TODO
}

int _osl_log_record_append(osl_od* od, void* data, uint16_t data_length) {
    osl_record_header_s rh;
    osl_object_s* object = osl_get_object(od);

    // Initialize
    rh.predecessor.subpage = 0;
    rh.predecessor.offset = 0;
    rh.is_first = 0;
    rh.length = data_length;

    if(object->num_objects == 0) {
        rh.is_first = true;
    } else {
        rh.predecessor.subpage = object->tail.subpage;
        rh.predecessor.offset = object->tail.offset;
    }

    int record_offset = _osl_buffer_write(od->osl, &rh, data);

    // Buffer is full :(
    if(record_offset == -EIO) {
        int ret = _osl_buffer_flush(od->osl);
        if(ret < 0) {
            return ret;
        }
    } else if(record_offset < 0) {
        return -EIO;
    }

    object->tail.subpage = od->osl->next_data_page;
    object->tail.offset = record_offset;
    object->num_objects++;

    return 0;
}

int _osl_log_record_get(osl_od* od, void* object_buffer, unsigned long index) {
    osl_object_s* object = osl_get_object(od);
    if(index >= object->num_objects) {
        MYDEBUG("Requested stream object with index out of bounds.\n");
        return -EFAULT;
    }

    // Number of objects written after target index
    int steps_back = object->num_objects - 1 - index;
    MYDEBUG("Steps to take: %d\n", steps_back);

    osl_record_header_s rh;
    osl_record_s record = object->tail;
    while(true) {
        if(steps_back <= 0) {
            MYDEBUG("Reached target record!\n");
            break;
        }

        int ret = _osl_record_header_get(od->osl, &record, &rh);
        if(ret != 0) {
            MYDEBUG("Retrieving header failed.\n");
            return ret;
        }

        steps_back -= rh.length / object->object_size;
        record = rh.predecessor;
    }

    if(steps_back != 0) {
        MYDEBUG("Record contained multiple objects. Not implemented yet.");
        assert(false);
    }

    int ret = _osl_record_datum_get(od->osl, &record, object_buffer, 0, object->object_size);
    return ret;
}










int osl_init(osl_s *osl, ftl_device_s *device) {
    MYDEBUG("Initializing OSL\n");
    osl->device = device;

    if(!ftl_is_initialized(device)) {
        MYDEBUG("FTL was not initialized\n");
        return -ENODEV;
    }

    osl->page_buffer_size = ftl_data_per_subpage(osl->device, true);
    osl->page_buffer_cursor = 0;
    osl->page_buffer = malloc(osl->page_buffer_size);
    if(osl->page_buffer == 0) {
        MYDEBUG("Couldn't allocate page buffer\n");
        return -ENOMEM;
    }

    int64_t first_index_page = _find_first_index_page(osl);
    if(first_index_page < 0) {
        osl->next_data_page = 0;
        osl->next_index_page = 0;
        osl->latest_index.first_page = 0;
    } else {
        // restore index state to memory
        return -1;
    }

    osl->open_objects = 0;
    return 0;
}

int osl_stream(osl_s* osl, osl_od* od, char* name, size_t object_size) {

    if(osl->open_objects >= OSL_MAX_OPEN_OBJECTS) {
        MYDEBUG("Cannot create new stream. Too many open objects.\n");
        return -EMFILE;
    }

    if(strlen(name) >= OSL_MAX_NAME_LENGTH) {
        MYDEBUG("Cannot create new stream. Name too long.\n");
        return -ENAMETOOLONG;
    }

    // TODO: Make sure to check if the object already exists!!!

    osl_object_s* obj = &osl->objects[osl->open_objects];
    memcpy(obj->name, name, strlen(name)+1);
    obj->object_size = object_size;
    obj->num_objects = 0;

    od->osl = osl;
    od->index = osl->open_objects;
    osl->open_objects++;

    return 0;
}

int osl_stream_append(osl_od* od, void* item) {
    osl_object_s* object = osl_get_object(od);
    return _osl_log_record_append(od, item, object->object_size);
}

int osl_stream_get(osl_od* od, void* object_buffer, unsigned long index) {
}











osl_object_s* osl_get_object(osl_od* od) {
    return &od->osl->objects[od->index];
}

