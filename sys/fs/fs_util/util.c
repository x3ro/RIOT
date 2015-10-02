#include <stdint.h>

#include "fs/util.h"

void fs_util_calc_flash_reading(fs_flash_reading *r, uint32_t size, uint32_t offset, uint32_t page_size) {
    r->start_page = offset / page_size;
    r->start_offset = offset % page_size;

    r->pages_to_read = size / page_size;
    if((size % page_size) > 0) {
        r->pages_to_read++;
    }

    if(r->pages_to_read == 1) {
        r->last_page_length = size;
    } else {
        r->last_page_length = size - ((r->pages_to_read - 1)  * page_size) - r->start_offset;
    }
}
