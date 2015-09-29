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
 * @brief       Default options for Coffee FS integration
 *
 * @author      Lucas Jenß <lucas@x3ro.de>
 */

#ifndef COFFEE_DEFAULTS_H
#define COFFEE_DEFAULTS_H

// Maximum length of a filename
#ifndef COFFEE_NAME_LENGTH
#define COFFEE_NAME_LENGTH 16
#endif

#ifndef COFFEE_MAX_OPEN_FILES
#define COFFEE_MAX_OPEN_FILES 6
#endif

#ifndef COFFEE_FD_SET_SIZE
#define COFFEE_FD_SET_SIZE 8
#endif

#ifndef COFFEE_LOG_SIZE
#define COFFEE_LOG_SIZE 8192
#endif

#ifndef COFFEE_LOG_TABLE_LIMIT
#define COFFEE_LOG_TABLE_LIMIT 256
#endif

/* Micro logs enable modifications on storage types that do not support
   in-place updates. This applies primarily to flash memories. */
#ifndef COFFEE_MICRO_LOGS
#define COFFEE_MICRO_LOGS 0
#endif

/* I/O semantics can be set on file descriptors in order to optimize
   file access on certain storage types. */
#ifndef COFFEE_IO_SEMANTICS
#define COFFEE_IO_SEMANTICS 0
#endif

/* If the files are expected to be appended to only, this parameter
   can be set to save some code space. */
#ifndef COFFEE_APPEND_ONLY
#define COFFEE_APPEND_ONLY  0
#endif

/*
 * Prevent sectors from being erased directly after file removal.
 * This will level the wear across sectors better, but may lead
 * to longer garbage collection procedures.
 */
#ifndef COFFEE_EXTENDED_WEAR_LEVELLING
#define COFFEE_EXTENDED_WEAR_LEVELLING  1
#endif

#endif
