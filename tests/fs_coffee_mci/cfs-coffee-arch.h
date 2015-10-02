/*
 * Copyright (C) 2015 Lucas Jenß
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup tests
 * @{
 *
 * @file
 * @brief       Configuration for the Coffee FS in-memory test
 *
 * @author      Lucas Jenß <lucas@x3ro.de>
 *
 * @}
 */

#ifndef MAIN_H
#define MAIN_H

// Sectors (also called blocks or erase segments). The erase operation can
// only be performed on a block-wise basis.
#define COFFEE_SECTOR_SIZE 524288UL

// Smallest unit than can be read/written (in MCI terminology, this would be a sector)
#define COFFEE_PAGE_SIZE 512UL

// Start address of the filesystem in bytes. Must point to the first byte in a sector.
#define COFFEE_START 0

// Total size (in bytes) available on the FS. This is used to calculate the amount
// of sectors and pages available
// #define COFFEE_SIZE ((3842048UL * COFFEE_PAGE_SIZE) - COFFEE_START)
#define COFFEE_SIZE ((2UL * COFFEE_SECTOR_SIZE) - COFFEE_START)

// Amount of bytes that are pre-allocated (i.e. reserved) when a new file is created
#define COFFEE_DYN_SIZE (16 * COFFEE_PAGE_SIZE)

#include "fs/coffee-defaults.h"
#include "fs/coffee-mci.h"

#endif
