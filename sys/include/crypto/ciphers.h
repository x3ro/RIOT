/*
 * Copyright (C) 2013 Freie Universität Berlin, Computer Systems & Telematics
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     sys_crypto
 * @{
 *
 * @file        ciphers.h
 * @brief       Headers for the packet encryption class. They are used to encrypt single packets.
 *
 * @author      Freie Universitaet Berlin, Computer Systems & Telematics
 * @author      Nicolai Schmittberger <nicolai.schmittberger@fu-berlin.de>
 * @author      Zakaria Kasmi <zkasmi@inf.fu-berlin.de>
 * @author      Mark Essien <markessien@gmail.com>
 */

#ifndef __CIPHERS_H_
#define __CIPHERS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Shared header file for all cipher algorithms */

/* Set the algorithms that should be compiled in here. When these defines
 * are set, then packets will be compiled 5 times.
 *
 * */
#define AES
// #define RC5
// #define THREEDES
// #define AES
// #define TWOFISH
// #define SKIPJACK

/** @brief the length of keys in bytes */
#define CIPHERS_MAX_KEY_SIZE 20
#define CIPHER_MAX_BLOCK_SIZE 16

/**
 * error codes
 */
#define CIPHER_ERR_UNSUPPORTED_CIHPER -2
#define CIPHER_ERR_INVALID_KEY_SIZE   -3
#define CIPHER_ERR_INVALID_LENGTH     -4
#define CIPHER_ERR_ENC_FAILED         -5
#define CIPHER_ERR_DEC_FAILED         -6

/**
 * @brief   the context for cipher-operations
 *          always order by number of bytes descending!!! <br>
 * rc5          needs 104 bytes                           <br>
 * threedes     needs 24  bytes                           <br>
 * aes          needs CIPHERS_MAX_KEY_SIZE bytes          <br>
 * twofish      needs CIPHERS_MAX_KEY_SIZE bytes          <br>
 * skipjack     needs 20 bytes                            <br>
 * identity     needs 1  byte                             <br>
 */
typedef struct {
#if defined(RC5)
    uint8_t context[104];             /**< supports RC5 and lower */
#elif defined(THREEDES)
    uint8_t context[24];              /**< supports ThreeDES and lower */
#elif defined(AES)
    uint8_t context[CIPHERS_MAX_KEY_SIZE]; /**< supports AES and lower */
#elif defined(TWOFISH)
    uint8_t context[CIPHERS_MAX_KEY_SIZE]; /**< supports TwoFish and lower */
#elif defined(SKIPJACK)
    uint8_t context[20];              /**< supports SkipJack and lower */
#endif
} cipher_context_t;


/**
 * @brief   BlockCipher-Interface for the Cipher-Algorithms
 */
typedef struct cipher_interface_st {
    /** Blocksize of this cipher */
    uint8_t block_size;

    /** Maximum key size for this cipher */
    uint8_t max_key_size;

    /** the init function */
    int (*init)(cipher_context_t* ctx, uint8_t block_size, const uint8_t* key,
                uint8_t key_size);

    /** the encrypt function */
    int (*encrypt)(const cipher_context_t* ctx, const uint8_t* plain_block,
                   uint8_t* cipher_block);

    /** the decrypt function */
    int (*decrypt)(const cipher_context_t* ctx, const uint8_t* cipher_block,
                   uint8_t* plain_block);

    /** the set_key function */
    int (*set_key)(cipher_context_t* ctx, const uint8_t* key, uint8_t key_size);
} cipher_interface_t;


/**
 * @brief   Numerical IDs for each cipher
 */
typedef enum {
    CIPHER_UNKNOWN,
    CIPHER_NULL,
    CIPHER_RC5,
    CIPHER_3DES,
    CIPHER_AES_128,
    CIPHER_TWOFISH,
    CIPHER_SKIPJACK
} cipher_id_t;

/**
 * @brief   cipher entry
 */
typedef struct cipher_entry_st {
    const char* name;
    cipher_id_t id;
    const cipher_interface_t* interface;
    uint8_t block_size;
} cipher_entry_t;


/**
 * @brief   list of all supported ciphers
 */
extern const cipher_entry_t cipher_list[];


/**
 * @brief basic struct for using block ciphers
 *        contains the cipher interface and the context
 */
typedef struct {
    const cipher_interface_t* interface;
    cipher_context_t context;
} cipher_t;


/**
 * @brief Initialize new cipher state
 *
 * @param cipher     cipher struct to init (already allocated memory)
 * @param cipher_id  cipher algorithm id
 * @param key        encryption key to use
 * @param len        length of the encryption key
 */
int cipher_init(cipher_t* cipher, cipher_id_t cipher_id, const uint8_t* key,
                uint8_t key_size);


/**
 * @brief set new encryption key for a cipher
 *
 * @param cipher     cipher struct to use
 * @param key        new encryption key
 * @param len        length of the new encryption key
 */
int cipher_set_key(cipher_t* cipher, const uint8_t* key, uint8_t key_size);


/**
 * @brief Encrypt data of BLOCK_SIZE length
 * *
 *
 * @param cipher     Already initialized cipher struct
 * @param input      pointer to input data to encrypt
 * @param output     pointer to allocated memory for encrypted data. It has to
 *                   be of size BLOCK_SIZE
 */
int cipher_encrypt(const cipher_t* cipher, const uint8_t* input, uint8_t* output);


/**
 * @brief Decrypt data of BLOCK_SIZE length
 * *
 *
 * @param cipher     Already initialized cipher struct
 * @param input      pointer to input data (of size BLOCKS_SIZE) to decrypt
 * @param output     pointer to allocated memory for decrypted data. It has to
 *                   be of size BLOCK_SIZE
 */
int cipher_decrypt(const cipher_t* cipher, const uint8_t* input, uint8_t* output);


/**
 * @brief Get block size of cipher
 * *
 *
 * @param cipher     Already initialized cipher struct
 */
int cipher_get_block_size(const cipher_t* cipher);


#ifdef __cplusplus
}
#endif

/** @} */
#endif /* __CIPHERS_H_ */
