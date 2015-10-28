#include "fs/ftl.h"
#include "fs/flash_sim.h"



flash_sim fs;

ftl_error_t ftl_init(void) {
    fs.page_size = FTL_PAGE_SIZE;
    fs.block_size = FTL_BLOCK_SIZE;
    fs.storage_size = 1024*1024*64; // 64MiB
    int ret = flash_sim_init(&fs);
    if(ret != E_SUCCESS) {
        return E_FTL_ERROR;
    }
    return E_FTL_SUCCESS;
}

ftl_error_t ftl_erase(blockptr_t block) {
    int ret = flash_sim_erase(&fs, block);
    if(ret != E_SUCCESS) {
        return E_FTL_ERROR;
    }
    return E_FTL_SUCCESS;
}

ftl_error_t ftl_read(const ftl_partition_s *partition,
                     char *buffer,
                     subpageptr_t subpage) {
    int ret;

    uint32_t page = partition->base_offset + (subpage / FTL_SUBPAGES_PER_PAGE);
    printf("reading from page %u\n", page);

    ret = flash_sim_read_partial(&fs, buffer, page, subpage * FTL_SUBPAGE_SIZE, FTL_SUBPAGE_SIZE);
    if(ret != E_SUCCESS) {
        return E_FTL_ERROR;
    }

    return E_FTL_SUCCESS;
}


