/*
 * Copyright (C) 2015 Lucas Jenß
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     sys_fs
 * @{
 *
 * @file
 * @brief       In-memory support for the Coffee FS
 *
 * @author      Lucas Jenß <lucas@x3ro.de>
 */

#ifndef CFS_COFFEE_ARCH_H
#define CFS_COFFEE_ARCH_H

#include <stdint.h>

// This macro should implement writing to flash memory. Note that size and offset are not
// limited to page boundaries. E.g. if the page size is 512 bytes, COFFEE_WRITE may be invoked
// with size = 1024 and offset = 26. The implementation of COFFEE_WRITE must make sure that
// the underlying flash memory is correctly adressed and possibly also erased!
#define COFFEE_WRITE(buf, size, offset)             \
        memcpy(memory + COFFEE_START + offset, (char *)(buf), size)

// Implements reading from flash memory. The note for COFFEE_WRITE also holds for this macro.
#define COFFEE_READ(buf, size, offset)              \
        memcpy((char *)(buf), memory + COFFEE_START + (offset), size)

#define COFFEE_ERASE(sector)                    \
        memset(memory + COFFEE_START + (sector) * COFFEE_SECTOR_SIZE, 0xff, COFFEE_SECTOR_SIZE)

typedef int16_t coffee_page_t;
static char memory[COFFEE_SIZE];

#endif
