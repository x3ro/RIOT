#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "fs/ftl.h"
#include "fs/flash_sim.h"

#define ENABLE_DEBUG (1)
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

uint8_t ftl_ecc_size(const ftl_device_s *device) {
    return (uint8_t) uint32_log2(device->subpage_size * 8) * 2;
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
    if(ecc_enabled) {
        return -1; // TODO
    } else {
        return device->subpage_size - sizeof(subpageheader_s);
    }
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

ftl_error_t ftl_read_raw(const ftl_partition_s *partition,
                     char *buffer,
                     subpageptr_t subpage) {

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
    if(data_length > partition->device->subpage_size - sizeof(header)) {
        return E_FTL_TOO_MUCH_DATA;
    }

    MYDEBUG("Writing to subpage %d\n", subpage);

    header.data_length = data_length;
    header.ecc_size = 0;
    header.flags = 0;

    memcpy(partition->device->page_buffer, &header, sizeof(header));
    memcpy(partition->device->page_buffer + sizeof(header), buffer, data_length);
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

    // TODO: check if there's an ECC!
    memcpy(buffer, partition->device->page_buffer + sizeof(subpageheader_s), header->data_length);
    return E_FTL_SUCCESS;
}
