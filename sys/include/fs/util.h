#ifndef FS_UTIL_H
#define FS_UTIL_H

#include <stdint.h>

typedef struct fs_flash_reading {
    uint32_t start_page;
    uint32_t start_offset;
    uint32_t pages_to_read;
    uint32_t last_page_length;
} fs_flash_reading;

void fs_util_calc_flash_reading(fs_flash_reading *r, uint32_t size, uint32_t offset, uint32_t page_size);

#endif
