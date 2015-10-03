#include <stdint.h>

#include "fs/util.h"

void fs_util_calc_flash_op(fs_flash_op *r, uint32_t size, uint32_t offset, uint32_t page_size) {
    r->start_page = offset / page_size;
    r->start_offset = offset % page_size;

    r->pages = size / page_size;
    if((size % page_size) > 0) {
        r->pages++;
    }

    if(r->pages == 1) {
        r->last_page_offset = size;
    } else {
        r->last_page_offset = size - ((r->pages - 1)  * page_size) - r->start_offset;
    }
}
