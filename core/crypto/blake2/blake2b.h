/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

// taken from here
// https://github.com/mjosaarinen/blake2_mjosref/blob/master/blake2b.h

#ifndef CORE_BLAKE2B_H
#define CORE_BLAKE2B_H

#include <cstddef>
#include <cstdint>

#include "common/blob.hpp"
#include "common/buffer.hpp"

namespace kagome::crypto {

  // state context
  // NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
  using blake2b_ctx = struct {
    uint8_t b[128];  // input buffer
    uint64_t h[8];   // chained state
    uint64_t t[2];   // total number of bytes
    size_t c;        // pointer for b[]
    size_t outlen;   // digest size
  };
  // NOLINTEND(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)

  // Initialize the hashing context "ctx" with optional key "key".
  //      1 <= outlen <= 64 gives the digest size in bytes.
  //      Secret key (also <= 64 bytes) is optional (keylen = 0).
  int blake2b_init(blake2b_ctx *ctx,
                   size_t outlen,
                   const void *key,
                   size_t keylen);  // secret key

  // Add "inlen" bytes from "in" into the hash.
  void blake2b_update(blake2b_ctx *ctx,  // context
                      const void *in,
                      size_t inlen);  // data to be hashed

  // Generate the message digest (size given in init).
  //      Result placed in "out".
  void blake2b_final(blake2b_ctx *ctx, void *out);

  // All-in-one convenience function.
  int blake2b(void *out,
              size_t outlen,  // return buffer for digest
              const void *key,
              size_t keylen,  // optional secret key
              const void *in,
              size_t inlen);  // data to be hashed

  template <size_t N>
  inline common::Blob<N> blake2b(common::BufferView buf) {
    common::Blob<N> out;
    BOOST_VERIFY(blake2b(out.data(), N, nullptr, 0, buf.data(), buf.size())
                 == 0);
    return out;
  }

}  // namespace kagome::crypto

#endif
