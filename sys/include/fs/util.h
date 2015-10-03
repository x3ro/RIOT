#ifndef FS_UTIL_H
#define FS_UTIL_H

#include <stdint.h>

/**
 * @brief Describes a read or write operation on flash memory.
 *
 *                                 pages (e.g. 2)
 *                    v-------------------------------------v
 * +------------------+------------------+------------------+------------------+
 * |       Page       |       Page       |       Page       |       Page       |
 * +------------------+------------------+------------------+------------------+
 *                    ^       ^                    ^
 *                    |       |                    |
 *               start_page   |             last_page_offset
 *                            |
 *                      start_offset
 */
typedef struct fs_flash_op {
    uint32_t start_page;        /**< Page at which the operation should begin (0-indexed) */
    uint32_t start_offset;      /**< Offset inside the first page at which the operation
                                     should begin  */
    uint32_t pages;             /**< Number of pages involved in the operation */
    uint32_t last_page_offset;  /**< Offset inside the last page at which the operation
                                    should end */
    uint32_t page_size;         /**< Page size of the underlying flash storage */
} fs_flash_op;

/**
 * @brief Compute low-level parameters for an operation on flash memory from memcpy-like
 *        higher level parameters.
 *
 * @param[out]  op
 * @param[in]   size        Number of bytes to be involved in the operation
 * @param[in]   offset      Absolute byte offset at which the operation should start
 * @param[in]   page_size   Page size of the underlying flash storage
 */
void fs_util_calc_flash_op(fs_flash_op *op, uint32_t size, uint32_t offset, uint32_t page_size);

#endif
