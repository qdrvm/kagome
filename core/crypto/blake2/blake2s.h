/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CORE_BLAKE2S_HASH
#define CORE_BLAKE2S_HASH

#include <stdlib.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
  unsigned char opaque[128];
} blake2s_ctx;

/**
 * @brief Initialize hash context
 * @param ctx context
 */
void blake2s_256_init(blake2s_ctx *ctx);

/**
 * @brief Update context with incoming bytes
 * @param ctx context
 * @param in {@param inlen} byte array
 * @param inlen size of {@param in}
 */
void blake2s_update(blake2s_ctx *ctx, const void *in, size_t inlen);

/**
 * @brief Finalize hash calculation
 * @param ctx context
 * @param out 32-byte output
 * @return
 */
void blake2s_final(blake2s_ctx *ctx, void *out);

/**
 * @brief One-shot convenience function to calculate blake2s_256 hash
 * @param out 32-byte buffer
 * @param in {@param inlen} bytes input buffer
 * @param inlen size of the input buffer
 * @return
 */
void blake2s_256(void *out, const void *in, size_t inlen);

/**
 * @brief Generic blake2s init function
 * @param ctx context
 * @param outlen 1..32 bytes of output buffer size
 * @param key optional key
 * @param keylen length of {@param key} in bytes. Pass 0 to indicate that key is
 * not provided.
 * @return -1 in case of wrong input arguments; 0 in case of success.
 */
int blake2s_init(blake2s_ctx *ctx, size_t outlen, const void *key,
                 size_t keylen);

/**
 * @brief All in one blake2s hashing function.
 * @param out output buffer
 * @param outlen size of {@param out}
 * @param key optional key
 * @param keylen size of {@param key}. Pass 0 to indicate that key is not
 * provided.
 * @param in data to be hashed
 * @param inlen size of {@param in}
 * @return -1 in case of wrong input arguments; 0 in case of success
 */
int blake2s(void *out, size_t outlen, const void *key, size_t keylen,
            const void *in, size_t inlen);

#if defined(__cplusplus)
}
#endif

#endif  // CORE_BLAKE2S_HASH
