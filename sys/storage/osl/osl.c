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
        return 0;
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
    //osl_collection_s* collection = osl_get_collection(cd);

    int record_offset = osl->page_buffer_cursor;

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



int _osl_record_datum_get(osl_s* osl, osl_record_s* record, void* datum, int offset, int size) {
    // TODO: Implement for when record is not held in page buffer anymore
    assert(record->subpage == 0);

    if(record->subpage == 0) {
        return _osl_buffer_read_datum(osl, record, datum, offset, size);
    }

    return -1; // TODO
}












int osl_init(osl_s *osl, ftl_device_s *device) {
    MYDEBUG("Initializing OSL\n");
    osl->device = device;

    osl->page_buffer_size = ftl_data_per_subpage(osl->device, true);
    osl->page_buffer_cursor = 0;
    osl->page_buffer = malloc(osl->page_buffer_size);
    if(osl->page_buffer == 0) {
        MYDEBUG("Couldn't allocate page buffer\n");
        return -ENOMEM;
    }

    osl->latest_index.first_page = _find_first_index_page(osl);
    osl->open_collections = 0;
    return 0;
}

osl_cd osl_stream_new(osl_s* osl, char* name, size_t object_size) {
    osl_cd cd;
    cd.osl = osl;

    if(osl->open_collections >= OSL_MAX_OPEN_COLLECTIONS) {
        MYDEBUG("Cannot create new stream. Too many open collections.\n");
        cd.index = -EMFILE;
        return cd;
    }

    if(strlen(name) > OSL_MAX_NAME_LENGTH) {
        MYDEBUG("Cannot create new stream. Name too long.\n");
        cd.index = -ENAMETOOLONG;
        return cd;
    }

    // TODO: Make sure to check if the collection already exists!!!

    osl_collection_s* obj = &osl->collections[osl->open_collections];
    memcpy(obj->name, name, strlen(name)+1);
    obj->object_size = object_size;
    obj->num_objects = 0;

    cd.index = osl->open_collections;
    osl->open_collections++;

    return cd;
}

int osl_stream_append(osl_cd* cd, void* item) {
    osl_record_header_s rh;
    osl_collection_s* collection = osl_get_collection(cd);

    rh.predecessor.subpage = 0;
    rh.predecessor.offset = 0;
    rh.is_first = 0;
    rh.length = collection->object_size;

    if(collection->num_objects == 0) {
        rh.is_first = 1;
    } else {
        rh.predecessor.subpage = collection->tail.subpage;
        rh.predecessor.offset = collection->tail.offset;
    }

    int record_offset = _osl_buffer_write(cd->osl, &rh, item);
    if(record_offset < 0) {
        return -EIO; // TODO proper error code
    }
    collection->tail.subpage = 0;
    collection->tail.offset = record_offset;
    collection->num_objects++;

    return 0;
}

int osl_stream_get(osl_cd* cd, void* object_buffer, unsigned long index) {
    osl_collection_s* collection = osl_get_collection(cd);
    if(index >= collection->num_objects) {
        MYDEBUG("Requested stream object with index out of bounds.\n");
        return -EFAULT;
    }

    // Number of objects written after target index
    int steps_back = collection->num_objects - 1 - index;
    MYDEBUG("Steps to take: %d\n", steps_back);

    osl_record_header_s rh;
    osl_record_s record = collection->tail;
    while(true) {
        if(steps_back <= 0) {
            MYDEBUG("Reached target record!\n");
            break;
        }

        int ret = _osl_record_header_get(cd->osl, &record, &rh);
        if(ret != 0) {
            MYDEBUG("Retrieving header failed.\n");
            return ret;
        }

        steps_back -= rh.length / collection->object_size;
        record = rh.predecessor;
    }

    if(steps_back != 0) {
        MYDEBUG("Record contained multiple objects. Not implemented yet.");
        assert(false);
    }

    int ret = _osl_record_datum_get(cd->osl, &record, object_buffer, 0, collection->object_size);
    return ret;
}











osl_collection_s* osl_get_collection(osl_cd* cd) {
    return &cd->osl->collections[cd->index];
}

