/*
 * Copyright (C) 2015 Lucas Jenß
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     unittests
 * @{
 *
 * @file
 * @brief       Tests for FS util module
 *
 * @author      Lucas Jenß <lucas@x3ro.de>
 *
 * @}
 */

#ifndef TESTS_FSUTIL_H_
#define TESTS_FSUTIL_H_

#include "embUnit.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   The entry point of this test suite.
 */
void tests_fsutil(void);

/**
 * @brief   Generates tests for hashes/md5.h
 *
 * @return  embUnit tests if successful, NULL if not.
 */
Test *tests_fsutil_tests(void);

#ifdef __cplusplus
}
#endif

#endif
