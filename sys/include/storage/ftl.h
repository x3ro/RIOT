/*
 * Copyright (C) 2015 Lucas Jenß
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_storage_ftl Flash Translation Layer
 * @ingroup     sys_storage
 *
 * @brief       Flash Translation Layer (FTL)
 *
 * @{
 * @author      Lucas Jenß <lucas@x3ro.de>
 */

#ifndef _FS_FTL_H
#define _FS_FTL_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @name Data Types
 * @{
 */

/**
 * @brief Error codes for FTL operations.
 */
typedef enum {
    E_FTL_SUCCESS = 0,               //!< Successful operation
    E_FTL_ERROR = 1,                 //!< Generic error
    E_FTL_INSUFFICIENT_STORAGE = 2,  //!< Not enough storage
    E_FTL_OUT_OF_RANGE = 3,          //!< Operation out of range (e.g. invalid page)
    E_FTL_TOO_MUCH_DATA = 4,         //!< Writing too much data (e.g. more than fits a single page)
    E_FTL_OUT_OF_MEMORY = 5,         //!< Out of memory (e.g. malloc fails)
    E_CORRUPT_PAGE = 6               //!< When page is damaged beyond repair w/ ECC
} ftl_error_t;

typedef uint32_t pageptr_t;         //!< Zero-indexed pointer to a page.
typedef uint32_t subpageptr_t;      //!< Zero-indexed pointer to a subpage.
typedef uint32_t blockptr_t;        //!< Zero-indexed pointer to block.

/* TODO: Remove this? */
typedef uint16_t subpageoffset_t;



// This is needed here since `ftl_partition_s` and `ftl_device_s` reference each other
struct ftl_device_s;

/**
 * @brief Describes a partition on a device managed by the FTL.
 */
typedef struct {
    struct ftl_device_s *device;    //!< The device on which the partition is located
    uint32_t base_offset;           //!< Zero-indexed absolute offset of the partition __in blocks__.
    uint32_t size;                  //!< Size of the partition __in blocks__.
} ftl_partition_s;

/**
 * @brief Describes a device managed by the FTL.
 */
typedef struct ftl_device_s {
    /**
     * Callback which must write a data buffer of the given length to a certain offset
     * inside a page.
     */
    ftl_error_t (*write)(const char *buffer,
                         pageptr_t page,
                         uint32_t offset,
                         uint16_t length);

    /**
     * Callback which must read a data segment of the given length from a certain offset
     * inside a page and writes it to the given data buffer.
     */
    ftl_error_t (*read)(char *buffer,
                        pageptr_t page,
                        uint32_t offset,
                        uint16_t length);

    /**
     * Callback which must erase the given block.
     */
    ftl_error_t (*erase)(blockptr_t block);

    ftl_partition_s index_partition; //!< Partition on which the flash index is stored
    ftl_partition_s data_partition;  //!< Partition on which user data is stored

    // TODO rename to subpage buffer
    char *page_buffer;  //!< Buffer for subpage read/write operations.
    char *ecc_buffer;   //!< Buffer for ECC calculation

    uint32_t total_pages;       //!< Total amount of pages configured for the device
    uint16_t page_size;         //!< Page size configured for the device
    uint16_t subpage_size;      //!< Subpage size
    uint16_t pages_per_block;   //!< Amount of pages inside an erase segment (block)
    uint8_t ecc_size;           //!< Size of the ECC determined for device's subpage size
} ftl_device_s;

/**
 * @brief Header which precedes every subpage not written in raw mode.
 *
 * The header may be followed by an ECC of the size defined in ::ftl_device_s's ecc_size.
 */
typedef struct __attribute__((__packed__)) {
    unsigned int data_length:16;    //!< Length of the data written to this subpage
    unsigned int ecc_enabled:1;     //!< If the header is directly followed by an ECC
    unsigned int reserved:7;        //!< Reserved for future use
} subpageheader_s;

/** @} */



/**
 * @name Functions operating on the device
 * @{
 */

/**
 * Initializes instance of the FTL based on the passed device configuration
 * @param  device configuration
 * @return        E_FTL_SUCCESS or an #ftl_error_t code
 */
ftl_error_t ftl_init(ftl_device_s *device);



/**
 * @name Functions operating on partitions
 * @{
 */

/**
 * Reads a single subpage
 * @param  partition to be read from
 * @param  buffer    to be written to
 * @param  header    Buffer to which the subpage header data will be written
 * @param  subpage   Index of the subpage to read
 * @return           E_FTL_SUCCESS or an #ftl_error_t code
 */
ftl_error_t ftl_read(const ftl_partition_s *partition,
                     char *buffer,
                     subpageheader_s *header,
                     subpageptr_t subpage);

/**
 * Writes a single subpage, including its header, without error correction
 * @param  partition   to be written to
 * @param  buffer      to be written from
 * @param  subpage     Index of the subpage to write to
 * @param  data_length Amount of bytes to write to the subpage
 * @return             E_FTL_SUCCESS or an #ftl_error_t code
 */
ftl_error_t ftl_write(const ftl_partition_s *partition,
                      const char *buffer,
                      subpageptr_t subpage,
                      subpageoffset_t data_length);

/**
 * Same as #ftl_write, but adds an error correction code (ECC) to the written subpage
 * @see #ftl_write
 */
ftl_error_t ftl_write_ecc(const ftl_partition_s *partition,
                      const char *buffer,
                      subpageptr_t subpage,
                      subpageoffset_t data_length);


/**
 * Erases all blocks in a given partition
 * @param[in]  partition
 * @return     E_FTL_SUCCESS or the result of any failed erase operation
 */
ftl_error_t ftl_format(const ftl_partition_s *partition);

/**
 * Erases a single block of the given partition
 * @param  partition
 * @param  block     Block to be erased
 * @return     E_FTL_SUCCESS or an #ftl_error_t code
 */
ftl_error_t ftl_erase(const ftl_partition_s *partition, blockptr_t block);

/**
 * Writes the buffer to the given subpage __without a subpage header__. For
 * automatic header handling use ftl_write() or ftl_write_ecc().
 *
 * @param[in]  partition    which is to be written
 * @param[in]  buffer       which should be written. It must be at least the size
 *                          of a subpage.
 * @param[in]  subpage      that should be written
 *
 * @return E_FTL_SUCCESS or an error code
 */
ftl_error_t ftl_write_raw(const ftl_partition_s *partition,
                          const char *buffer,
                          subpageptr_t subpage);

/**
 * Reads a single subpage from the given partition into the buffer, __including
 * the subpage header__. For automatic header handling use ftl_read().
 *
 * @param[in]  partition from which to read
 * @param[out] buffer    to which subpage should be written. It must be
 *                       at least the size of a subpage.
 * @param[in]  subpage   to read
 *
 * @return E_FTL_SUCCESS or an error code
 */
ftl_error_t ftl_read_raw(const ftl_partition_s *partition,
                         char *buffer,
                         subpageptr_t subpage);



/**
 * @name Utility functions without side-effects
 * @{
 */

/**
 * Computes the required ECC size for the given device configuration
 * @param  device
 * @return        ECC size
 */
uint8_t ftl_ecc_size(const ftl_device_s *device);

/**
 * Computes the first subpage of a given block on the device (absolute, not per-partition)
 * @param  device
 * @param  block
 * @return        The absolute index of the first subpage in the given block
 */
subpageptr_t ftl_first_subpage_of_block(const ftl_device_s *device, blockptr_t block);

/**
 * Computes the amount of bytes that can be stored per subpage for the given device
 * @param  device
 * @param  ecc_enabled Whether ECC is enabled or not for this page
 * @return             Number of bytes that fit into the subpage
 */
subpageoffset_t ftl_data_per_subpage(const ftl_device_s *device, bool ecc_enabled);

/**
 * @param  partition
 * @return           The number of subpages which the given partition contains
 */
uint32_t ftl_subpages_in_partition(const ftl_partition_s *partition);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
/** @} */
