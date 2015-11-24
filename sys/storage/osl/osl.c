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
    ftl_read(&osl->device->index_partition, (char*)osl->page_buffer, &header, 0);

    bool formatted = true;
    for(int i=0; i < osl->device->subpage_size; i++) {
        printf("%x ", osl->page_buffer[i]);
        if(osl->page_buffer[i] != 0xff) {
            formatted = false;
            break;
        }
    }

    if(formatted) {
        MYDEBUG("Detected empty index partition. Assuming file system is empty\n");
        return 0;
    }

    return -1;
}


int osl_init(osl_s *osl, ftl_device_s *device) {
    MYDEBUG("Initializing OSL\n");
    osl->device = device;

    osl->page_buffer = malloc(ftl_data_per_subpage(osl->device, true));
    if(osl->page_buffer == 0) {
        MYDEBUG("Couldn't allocate page buffer\n");
        return -ENOMEM;
    }

    osl->latest_index.first_page = _find_first_index_page(osl);
    return 0;
}



