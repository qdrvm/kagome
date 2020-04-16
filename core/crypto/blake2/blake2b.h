/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CORE_BLAKE2B_H
#define CORE_BLAKE2B_H

#include <stddef.h>
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

// state context
typedef struct {
  uint8_t b[128];  // input buffer
  uint64_t h[8];   // chained state
  uint64_t t[2];   // total number of bytes
  size_t c;        // pointer for b[]
  size_t outlen;   // digest size
} blake2b_ctx;

// Initialize the hashing context "ctx" with optional key "key".
//      1 <= outlen <= 64 gives the digest size in bytes.
//      Secret key (also <= 64 bytes) is optional (keylen = 0).
int blake2b_init(blake2b_ctx *ctx, size_t outlen, const void *key,
                 size_t keylen);  // secret key

// Add "inlen" bytes from "in" into the hash.
void blake2b_update(blake2b_ctx *ctx,               // context
                    const void *in, size_t inlen);  // data to be hashed

// Generate the message digest (size given in init).
//      Result placed in "out".
void blake2b_final(blake2b_ctx *ctx, void *out);

// All-in-one convenience function.
int blake2b(void *out, size_t outlen,        // return buffer for digest
            const void *key, size_t keylen,  // optional secret key
            const void *in, size_t inlen);   // data to be hashed

#if defined(__cplusplus)
}
#endif

#endif
