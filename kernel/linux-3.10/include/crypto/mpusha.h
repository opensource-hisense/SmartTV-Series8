/*
 * (c) 1997-2010 Netflix, Inc.  All content herein is protected by
 * U.S. copyright and other applicable intellectual property laws and
 * may not be copied without the express permission of Netflix, Inc.,
 * which reserves all rights.  Reuse of any of this content for any
 * purpose without the permission of Netflix, Inc. is strictly
 * prohibited.
 */

/* 
 * Based off Crypto++ SHA-1 by Steve Reid.
 * http://sourceforge.net/projects/cryptopp/
 */
#ifndef H__SHA
#define H__SHA

#include <linux/types.h>

/** SHA-1 state. */
struct sha1_state;

/**
 * Create a new SHA-1 state. Be sure to free the state when finished.
 *
 * @return a SHA-1 state. May be NULL if the memory allocation failed.
 */
struct sha1_state *sha1_init(void);

/**
 * Feed some bits into the SHA-1.
 *
 * @param[in|out] state SHA-1 state.
 * @param[in] data data.
 * @param[in] len data length in bytes.
 */
void sha1_update(struct sha1_state *state, const uint8_t *data, uint32_t len);

/**
 * Feed some bits into the SHA-1 and compute the final digest.
 *
 * @param[in] state SHA-1 state.
 * @param[in] data data.
 * @param[in] len data length in bytes.
 * @return 160-bit SHA-1 digest. The caller is responsible for freeing the
 *         returned memory. May be NULL if the memory allocation failed.
 */
uint8_t *sha1_digest(struct sha1_state *state, const uint8_t *data, uint32_t len);

/**
 * Compute the final SHA-1 digest.
 *
 * @param[in] state SHA-1 state.
 * @return 160-bit SHA-1 digest. The caller is responsible for freeing the
 *         returned memory. May be NULL if the memory allocation failed.
 */
uint8_t *sha1_final(struct sha1_state *state);

/**
 * Free a SHA-1 state.
 *
 * @param[in] state SHA-1 state.
 */
void sha1_free(struct sha1_state *state);

/** SHA-256 state. */
struct sha256_state;

/**
 * Create a new SHA-256 state. Be sure to free the state when finished.
 *
 * @return a SHA-256 state. May be NULL if the memory allocation failed.
 */
struct sha256_state *sha256_init(void);

/**
 * Feed some bits into the SHA-256.
 *
 * @param[in|out] state SHA-256 state.
 * @param[in] data data.
 * @param[in] len data length in bytes.
 */
void sha256_update(struct sha256_state *state, const uint8_t *data, uint32_t len);

/**
 * Feed some bits into the SHA-256 and compute the final digest.
 *
 * @param[in] state SHA-256 state.
 * @param[in] data data.
 * @param[in] len data length in bytes.
 * @return 256-bit SHA-256 digest. The caller is responsible for
 *         freeing the returned memory. May be NULL if memory
 *         allocation failed.
 */
uint8_t *sha256_digest(struct sha256_state *state, const uint8_t *data, uint32_t len);

/**
 * Compute the final SHA-256 digest.
 *
 * @param[in] state SHA-256 state.
 * @return 256-bit SHA-256 digest. The caller is responsible for
 *         freeing the returned memory. May be NULL if the memory
 *         allocation failed.
 */
uint8_t *sha256_final(struct sha256_state *state);

/**
 * Free a SHA-256 state.
 *
 * @param[in] state SHA-256 state.
 */
void sha256_free(struct sha256_state *state);

#endif
