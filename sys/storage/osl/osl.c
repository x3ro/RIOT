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
    int ret = ftl_read(&osl->device->index_partition, (char*)osl->subpage_buffer, &header, 0);

    if(ret == -ENOENT) {
        MYDEBUG("Detected empty index partition. Assuming file system is empty\n");
        return -1;
    }

    // TODO: Implement index partition scan algorithm
    assert(false);
    return -1;
}

int _osl_buffer_write(osl_s* osl, osl_record_header_s* record, void* item) {
    int record_offset = osl->subpage_buffer_cursor;

    // Trying to write past the page buffer. It needs to be flushed.
    if( (record_offset + sizeof(osl_record_header_s) + record->length) >= osl->subpage_buffer_size ) {
        return -EIO;
    }

    MYDEBUG("Buffering record w/ predecessor offset %d and subpage %d to offset %d\n",
        record->predecessor.offset,
        record->predecessor.subpage,
        osl->subpage_buffer_cursor);

    memcpy( osl->subpage_buffer + osl->subpage_buffer_cursor,
            record,
            sizeof(osl_record_header_s));

    osl->subpage_buffer_cursor += sizeof(osl_record_header_s);

    memcpy( osl->subpage_buffer + osl->subpage_buffer_cursor,
            item,
            record->length);

    osl->subpage_buffer_cursor += record->length;

    return record_offset;
}

int _osl_buffer_read_header(unsigned char *buffer, osl_record_s* record, osl_record_header_s* rh) {
    memcpy( rh,
            buffer + record->offset,
            sizeof(osl_record_header_s));

    return 0;
}

int _osl_buffer_read_datum(unsigned char *buffer, osl_record_s* record, void* datum, int offset, int size) {
    memcpy( datum,
            buffer + record->offset + sizeof(osl_record_header_s) + offset,
            size);

    return 0;

}

int _osl_read_page(osl_s* osl, uint32_t subpage) {
    MYDEBUG("Reading subpage %d\n", subpage);
    subpageheader_s header;
    int ret = ftl_read(&osl->device->data_partition, (char*) osl->read_buffer, &header, subpage);
    if(ret != 0) {
        return ret;
    }

    osl->read_buffer_partition = OSL_DATA_PARTITION;
    osl->read_buffer_subpage = subpage;
    return 0;
}

int _osl_record_header_get(osl_s* osl, osl_record_s* record, osl_record_header_s* rh) {

    MYDEBUG("subpage %d, offset %d, next_data_subpage %d, read_buffer_subpage %d\n",
        record->subpage,
        record->offset,
        osl->next_data_subpage,
        osl->read_buffer_subpage);

    if(record->subpage == osl->next_data_subpage) {
        // Record is still in page buffer
        return _osl_buffer_read_header(osl->subpage_buffer, record, rh);

    } else if(  osl->read_buffer_partition == OSL_DATA_PARTITION &&
                osl->read_buffer_subpage == record->subpage) {
        // record is in read buffer
        return _osl_buffer_read_header(osl->read_buffer, record, rh);

    }

    // record is not available in any buffer, lets get it
    int ret = _osl_read_page(osl, record->subpage);
    if(ret != 0) {
        return ret;
    }

    // Now that the page is in the read buffer, we can safely recurse to get
    // the subpage
    return _osl_record_header_get(osl, record, rh);
}

int _osl_record_datum_get(osl_s* osl, osl_record_s* record, void* datum, int offset, int size) {

    if(record->subpage == osl->next_data_subpage) {
        return _osl_buffer_read_datum(osl->subpage_buffer, record, datum, offset, size);

    } else if(  osl->read_buffer_partition == OSL_DATA_PARTITION &&
                osl->read_buffer_subpage == record->subpage) {
        return _osl_buffer_read_datum(osl->read_buffer, record, datum, offset, size);

    }

    // TODO: Same logic as in _osl_record_header_get, refactor!

    // record is not available in any buffer, lets get it
    int ret = _osl_read_page(osl, record->subpage);
    if(ret != 0) {
        return ret;
    }

    // Now that the page is in the read buffer, we can safely recurse to get
    // the subpage
    return _osl_record_datum_get(osl, record, datum, offset, size);
}

int _osl_buffer_flush(osl_s* osl) {
    MYDEBUG("Flushing buffer to page %d\n", osl->next_data_subpage);

    int ret = ftl_write_ecc(&osl->device->data_partition,
                            (char *) osl->subpage_buffer,
                            osl->next_data_subpage,
                            osl->subpage_buffer_size);

    if(ret != 0) {
        return ret;
    }

    memset(osl->subpage_buffer, 0, osl->subpage_buffer_size);
    osl->subpage_buffer_cursor = 0;
    osl->next_data_subpage++;
    return 0;
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
    MYDEBUG("returned offset %d\n", record_offset);


    // Buffer is full :(
    if(record_offset == -EIO) {
        int ret = _osl_buffer_flush(od->osl);
        if(ret < 0) {
            return ret;
        }

        return _osl_log_record_append(od, data, data_length);
    } else if(record_offset < 0) {
        return -EIO;
    }

    object->tail.subpage = od->osl->next_data_subpage;
    object->tail.offset = record_offset;
    object->num_objects++;

    return 0;
}

int _osl_log_record_get(osl_od* od, void* object_buffer, unsigned long index) {
    osl_object_s* object = osl_get_object(od);
    if(index >= object->num_objects) {
        MYDEBUG("Requested record with index out of bounds.\n");
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

    osl->subpage_buffer_size = ftl_data_per_subpage(osl->device, true);
    osl->subpage_buffer_cursor = 0;
    osl->subpage_buffer = malloc(osl->subpage_buffer_size);
    osl->read_buffer = malloc(osl->subpage_buffer_size);
    if(osl->subpage_buffer == 0 || osl->read_buffer == 0) {
        MYDEBUG("Couldn't allocate page buffers\n");
        return -ENOMEM;
    }

    osl->read_buffer_partition = -1;
    osl->read_buffer_subpage = 0;

    int64_t first_index_page = _find_first_index_page(osl);
    if(first_index_page < 0) {
        osl->next_data_subpage = 0;
        osl->next_index_subpage = 0;
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
    return _osl_log_record_get(od, object_buffer, index);
}

osl_object_s* osl_get_object(osl_od* od) {
    return &od->osl->objects[od->index];
}

