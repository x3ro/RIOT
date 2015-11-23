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
 * @brief       Implementation of Flash Translation Layer (FTL)
 * @author      Lucas Jenß <lucas@x3ro.de>
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "storage/ftl.h"
#include "storage/flash_sim.h"
#include "ecc/hamming256.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

#define MYDEBUG(...) DEBUG("%s: ", __FUNCTION__); \
                     DEBUG(__VA_ARGS__)

#define FTL_INDEX_SIZE (1024*1024*4) // 4MiB

uint32_t uint32_log2(uint32_t inVal) {
    if(inVal == 0)
        return 0;
    uint32_t tempOut = 0;
    if(inVal >= (1 << 16)) { inVal >>= 16; tempOut += 16; }
    if(inVal >= (1 <<  8)) { inVal >>=  8; tempOut +=  8; }
    if(inVal >= (1 <<  4)) { inVal >>=  4; tempOut +=  4; }
    if(inVal >= (1 <<  2)) { inVal >>=  2; tempOut +=  2; }
    if(inVal >= (1 <<  1)) {               tempOut +=  1; }
    return tempOut;
}

uint64_t ftl_device_capacity(const ftl_device_s *device) {
    return device->total_pages * device->page_size;
}

uint32_t ftl_device_blocksize(const ftl_device_s *device) {
    return device->pages_per_block * device->page_size;
}

uint32_t ftl_subpages_in_partition(const ftl_partition_s *partition) {
    return partition->size * partition->device->pages_per_block *
           (partition->device->page_size / partition->device->subpage_size);
}

uint8_t ftl_ecc_size(const ftl_device_s *device) {
    // This would be the result for the "good" Hamming algorithm
    //return (uint8_t) uint32_log2(device->subpage_size * 8) * 2;

    // The current Hamming code implememntation unfortunately only supports
    // 256 byte blocks, each with a 22 bit code.
    uint8_t bitsize = (device->subpage_size / 256) * 22;

    uint8_t bytesize = bitsize / 8;
    if(bitsize % 8 > 0) {
        bytesize++;
    }

    return bytesize;
}

uint8_t ftl_subpage_mod(const ftl_device_s *device, subpageptr_t subpage) {
    return subpage % (device->page_size / device->subpage_size);
}

subpageptr_t ftl_first_subpage_of_block(const ftl_device_s *device, blockptr_t block) {
    return block * device->pages_per_block * (device->page_size / device->subpage_size);
}

pageptr_t ftl_subpage_to_page(const ftl_partition_s *partition, subpageptr_t subpage) {
    return (partition->base_offset * partition->device->pages_per_block) +
            subpage / (partition->device->page_size / partition->device->subpage_size);
}

subpageoffset_t ftl_data_per_subpage(const ftl_device_s *device, bool ecc_enabled) {
    subpageoffset_t size = device->subpage_size - sizeof(subpageheader_s);

    if(ecc_enabled) {
        size -= device->ecc_size;
    }

    return size;
}

ftl_error_t ftl_init(ftl_device_s *device) {
    if(ftl_device_capacity(device) < FTL_INDEX_SIZE) {
        return E_FTL_INSUFFICIENT_STORAGE;
    }

    uint32_t blocksize = ftl_device_blocksize(device);
    device->index_partition.device = device;
    device->index_partition.base_offset = 0;
    device->index_partition.size = FTL_INDEX_SIZE / blocksize;
    if((FTL_INDEX_SIZE % blocksize) > 0) {
        device->index_partition.size += 1;
    }

    // The data partition starts directly after the index and spans the rest
    // of the device.
    device->data_partition.device = device;
    device->data_partition.base_offset = device->index_partition.size;
    device->data_partition.size = (device->total_pages / device->pages_per_block) -
                                   device->index_partition.size;

    device->ecc_size = ftl_ecc_size(device);
    device->page_buffer = malloc(device->subpage_size);
    if(device->page_buffer == 0)  {
        return E_FTL_OUT_OF_MEMORY;
    }

    device->ecc_buffer = malloc(device->ecc_size);
    if(device->ecc_buffer == 0)  {
        return E_FTL_OUT_OF_MEMORY;
    }

    return E_FTL_SUCCESS;
}

ftl_error_t ftl_erase(const ftl_partition_s *partition, blockptr_t block) {
    blockptr_t absolute_block = partition->base_offset + block;
    uint32_t block_capacity = partition->device->total_pages / partition->device->pages_per_block;
    if(block >= partition->size || absolute_block >= block_capacity) {
        return E_FTL_OUT_OF_RANGE;
    }

    return partition->device->erase(absolute_block);
}

ftl_error_t ftl_format(const ftl_partition_s *partition) {
    uint32_t blocks = partition->size;
    ftl_error_t ret;
    for(uint32_t i=0; i<blocks; i++) {
        ret = ftl_erase(partition, i);
        if(ret != E_FTL_SUCCESS) {
            return ret;
        }
    }
    return ret;
}

ftl_error_t ftl_read_raw(const ftl_partition_s *partition,
                     char *buffer,
                     subpageptr_t subpage) {

    if(subpage >= ftl_subpages_in_partition(partition)) {
        return E_FTL_OUT_OF_RANGE;
    }

    pageptr_t page = ftl_subpage_to_page(partition, subpage);
    MYDEBUG("Reading from page %u, offset=%u, size=%u\n", page,
        ftl_subpage_mod(partition->device, subpage) * partition->device->subpage_size,
        partition->device->subpage_size);

    return partition->device->read(buffer,
                                   page,
                                   ftl_subpage_mod(partition->device, subpage) * partition->device->subpage_size,
                                   partition->device->subpage_size);
}

ftl_error_t ftl_write_raw(const ftl_partition_s *partition,
                     const char *buffer,
                     subpageptr_t subpage) {

    if(subpage >= ftl_subpages_in_partition(partition)) {
        return E_FTL_OUT_OF_RANGE;
    }

    pageptr_t page = ftl_subpage_to_page(partition, subpage);
    MYDEBUG("Writing to page %u, offset=%u, size=%u\n", page,
        ftl_subpage_mod(partition->device, subpage) * partition->device->subpage_size,
        partition->device->subpage_size);

     return partition->device->write(
        buffer,
        page,
        ftl_subpage_mod(partition->device, subpage) * partition->device->subpage_size,
        partition->device->subpage_size);
}


ftl_error_t ftl_write(const ftl_partition_s *partition,
                      const char *buffer,
                      subpageptr_t subpage,
                      subpageoffset_t data_length) {

    subpageheader_s header;
    if(data_length > ftl_data_per_subpage(partition->device, false)) {
        return E_FTL_TOO_MUCH_DATA;
    }

    MYDEBUG("Writing to subpage %d\n", subpage);

    header.data_length = data_length;
    header.ecc_enabled = 0;
    header.reserved = 0;

    memcpy(partition->device->page_buffer, &header, sizeof(header));
    memcpy(partition->device->page_buffer + sizeof(header), buffer, data_length);
    return ftl_write_raw(partition, partition->device->page_buffer, subpage);
}

ftl_error_t ftl_write_ecc(const ftl_partition_s *partition,
                      const char *buffer,
                      subpageptr_t subpage,
                      subpageoffset_t data_length) {

    subpageheader_s header;
    MYDEBUG("l: %d, other: %d\n", data_length, partition->device->subpage_size - ftl_data_per_subpage(partition->device, true));
    if(data_length >
       ftl_data_per_subpage(partition->device, true)) {
        return E_FTL_TOO_MUCH_DATA;
    }

    MYDEBUG("Writing to subpage %d w/ ECC\n", subpage);

    header.data_length = data_length;
    header.ecc_enabled = 1;
    header.reserved = 0;

    // Page buffer needs to be wiped because of the ECC calculation
    memset(partition->device->page_buffer, 0x00, partition->device->subpage_size);

    memcpy(partition->device->page_buffer, &header, sizeof(header));

    memcpy(partition->device->page_buffer + sizeof(header) + partition->device->ecc_size,
           buffer, data_length);

    Hamming_Compute256x((uint8_t*)partition->device->page_buffer,
                        partition->device->subpage_size,
                        (uint8_t*)partition->device->ecc_buffer);

    memcpy(partition->device->page_buffer + sizeof(header),
           partition->device->ecc_buffer,
           partition->device->ecc_size);

    return ftl_write_raw(partition, partition->device->page_buffer, subpage);
}

ftl_error_t ftl_read(const ftl_partition_s *partition,
                     char *buffer,
                     subpageheader_s *header,
                     subpageptr_t subpage) {

    ftl_error_t ret = ftl_read_raw(partition, partition->device->page_buffer, subpage);
    if(ret != E_FTL_SUCCESS) {
        return ret;
    }

    memcpy(header, partition->device->page_buffer, sizeof(subpageheader_s));
    if(header->data_length > partition->device->subpage_size) {
        return E_CORRUPT_PAGE;
    }

    char *data_buffer = partition->device->page_buffer;
    char *ecc_buffer = partition->device->ecc_buffer;
    if(header->ecc_enabled) {
        uint8_t ecc_size = partition->device->ecc_size;
        memcpy(ecc_buffer, data_buffer + sizeof(subpageheader_s), ecc_size);
        memset(data_buffer + sizeof(subpageheader_s), 0x00, ecc_size);
        uint8_t result = Hamming_Verify256x((uint8_t*) data_buffer,
                                            partition->device->subpage_size,
                                            (uint8_t*) ecc_buffer);

        if(result != Hamming_ERROR_NONE && result != Hamming_ERROR_SINGLEBIT) {
            return E_CORRUPT_PAGE;
        } else if(result == Hamming_ERROR_SINGLEBIT) {
            // We need to update the header in case that the flipped bit was in there
            memcpy(header, partition->device->page_buffer, sizeof(subpageheader_s));
        }

        data_buffer += ecc_size;
    }

    data_buffer += sizeof(subpageheader_s);

    memcpy(buffer, data_buffer, header->data_length);
    return E_FTL_SUCCESS;
}
