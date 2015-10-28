
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "fs/flash_sim.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#define MYDEBUG(...) DEBUG("%s: ", __FUNCTION__); \
                     DEBUG(__VA_ARGS__)

#define FLASH_SIM_FILENAME "flash_sim.dat"

bool _valid_struct_params(flash_sim *fs) {
    return fs->page_size > 0 &&
           fs->block_size > 0 &&
           fs->storage_size > 0 &&
           (fs->block_size % fs->page_size) == 0 &&
           (fs->storage_size & fs->block_size) == 0;
}

flash_sim_error_t flash_sim_init(flash_sim *fs) {
    if(!_valid_struct_params(fs)) {
        return E_INVALID_PARAMS;
    }

    fs->pages = fs->storage_size / fs->page_size;
    struct stat buffer;
    if(stat(FLASH_SIM_FILENAME, &buffer) == 0) {
        fs->_fp = fopen(FLASH_SIM_FILENAME, "r+");
    } else {
        fs->_fp = fopen(FLASH_SIM_FILENAME, "w+");
    }

    if(fs->_fp == NULL) {
        MYDEBUG("failed opening file %s\n", FLASH_SIM_FILENAME);
        return E_FILE_ERROR;
    }

    // Make sure that the file has exactly the right size
    fseek(fs->_fp, 0, SEEK_END);
    long size = ftell(fs->_fp);

    if(size == (long) fs->storage_size) {
        return E_SUCCESS;
    } else {
        return flash_sim_format(fs);
    }
}

flash_sim_error_t flash_sim_format(flash_sim *fs) {
    if(fs->_fp == NULL) {
        MYDEBUG("struct was not initialized\n");
        return E_NOT_INITIALIZED;
    }

    int ret = fclose(fs->_fp);
    if(ret != 0) {
        MYDEBUG("could not close file\n");
        return E_FILE_ERROR;
    }

    fs->_fp = fopen(FLASH_SIM_FILENAME, "w+");
    if(fs->_fp == NULL) {
        MYDEBUG("failed opening file %s\n", FLASH_SIM_FILENAME);
        return E_FILE_ERROR;
    }

    unsigned char *buffer = malloc(fs->storage_size);
    memset(buffer, 0xFF, fs->storage_size);
    fwrite(buffer, fs->storage_size, 1, fs->_fp);

    return E_SUCCESS;
}

flash_sim_error_t flash_sim_read(const flash_sim *fs, void *buffer, uint32_t page) {
    MYDEBUG("page = %u\n", page);

    if(fs->_fp == NULL) {
        MYDEBUG("struct was not initialized\n");
        return E_NOT_INITIALIZED;
    }

    int ret = fseek(fs->_fp, page * fs->page_size, SEEK_SET);
    if(ret != 0) {
        MYDEBUG("seek failed: out of range\n");
        return E_OUT_OF_RANGE;
    }

    ret = fread(buffer, fs->page_size, 1, fs->_fp);
    if(ret != 1) {
        MYDEBUG("read failed: out of range\n");
        return E_OUT_OF_RANGE;
    }

    return E_SUCCESS;
}

// flash_sim_error_t flash_sim_read(flash_sim *fs, void *buffer, uint32_t page, uint32_t offset, uint32_t length) {
//     unsigned char *page_buffer = malloc(fs->page_size);
//     flash_sim_read(fs)
// }
//
flash_sim_error_t flash_sim_read_partial(const flash_sim *fs, void *buffer, uint32_t page, uint32_t offset, uint32_t length) {
    unsigned char *page_buffer = malloc(fs->page_size);

    int ret = flash_sim_read(fs, page_buffer, page);
    if(ret != E_SUCCESS) {
        free(page_buffer);
        return ret;
    }

    memcpy(buffer, page_buffer + offset, length);
    free(page_buffer);
    return E_SUCCESS;
}

flash_sim_error_t flash_sim_write_partial(const flash_sim *fs, const void *buffer, uint32_t page, uint32_t offset, uint32_t length) {
    unsigned char *page_buffer = malloc(fs->page_size);
    int ret = flash_sim_read(fs, page_buffer, page);
    if(ret != E_SUCCESS) {
        MYDEBUG("read failed page = %u\n", page);
        free(page_buffer);
        return ret;
    }

    memcpy(page_buffer+offset, buffer, length);
    ret = flash_sim_write(fs, page_buffer, page);
    if(ret != E_SUCCESS) {
        MYDEBUG("write failed page = %u\n", page);
        free(page_buffer);
        return ret;
    }

    return E_SUCCESS;
}

flash_sim_error_t flash_sim_write(const flash_sim *fs, const void *buffer, uint32_t page) {
    MYDEBUG("page = %u\n", page);

    if(fs->_fp == NULL) {
        MYDEBUG("struct was not initialized\n");
        return E_NOT_INITIALIZED;
    }

    bool valid_write = true;
    unsigned char *page_buffer = malloc(fs->page_size);
    int ret = flash_sim_read(fs, page_buffer, page);
    if(ret != E_SUCCESS) {
        MYDEBUG("read failed page = %u\n", page);
        free(page_buffer);
        return ret;
    }
    // Make sure that its impossible to set bits back to 1 without erasing
    for(unsigned int i=0; i<fs->page_size; i++) {
        if((((unsigned char*) buffer)[i] | page_buffer[i]) > page_buffer[i]) {
            valid_write = false;
            break;
        }
    }
    free(page_buffer);

    if(!valid_write) {
        MYDEBUG("write failed - would have set bits back to 1\n");
        return E_INVALID_WRITE;
    }


    ret = fseek(fs->_fp, page * fs->page_size, SEEK_SET);
    if(ret != 0) {
        MYDEBUG("seek failed: out of range\n");
        return E_OUT_OF_RANGE;
    }

#if ENABLE_DEBUG
    long pos = ftell(fs->_fp);
    MYDEBUG("writing to offset %ld\n", pos);
#endif

    ret = fwrite(buffer, fs->page_size, 1, fs->_fp);
    if(ret != 1) {
        MYDEBUG("write failed\n");
        return E_FILE_ERROR;
    }

    return E_SUCCESS;
}

flash_sim_error_t flash_sim_erase(const flash_sim *fs, uint32_t block) {
    MYDEBUG("block = %u\n", block);

    if(fs->_fp == NULL) {
        MYDEBUG("struct was not initialized\n");
        return E_NOT_INITIALIZED;
    }

    unsigned char *buffer = malloc(fs->block_size);
    memset(buffer, 0xff, fs->block_size);

    int ret = fseek(fs->_fp, block * fs->block_size, SEEK_SET);
    if(ret != 0) {
        free(buffer);
        MYDEBUG("seek failed: out of range\n");
        return E_OUT_OF_RANGE;
    }

    ret = fwrite(buffer, fs->block_size, 1, fs->_fp);
    if(ret != 1) {
        free(buffer);
        MYDEBUG("write failed\n");
        return E_FILE_ERROR;
    }

    free(buffer);
    return E_SUCCESS;
}
