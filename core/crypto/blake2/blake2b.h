/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

// taken from here
// https://github.com/mjosaarinen/blake2_mjosref/blob/master/blake2b.h

#pragma once

#include <cstddef>
#include <cstdint>

#include "common/blob.hpp"
#include "common/buffer.hpp"

namespace kagome::crypto {

  // state context
  struct blake2b_ctx {
    std::array<uint8_t, 128> b;  // input buffer
    std::array<uint64_t, 8> h;   // chained state
    std::array<uint64_t, 2> t;   // total number of bytes
    size_t c;                    // pointer for b[]
    size_t outlen;               // digest size
  };

  // Initialize the hashing context "ctx" with optional key "key".
  //      1 <= outlen <= 64 gives the digest size in bytes.
  //      Secret key (also <= 64 bytes) is optional (keylen = 0).
  int blake2b_init(blake2b_ctx &ctx,
                   size_t outlen,
                   const void *key,
                   size_t keylen);  // secret key

  // Add "inlen" bytes from "in" into the hash.
  void blake2b_update(blake2b_ctx &ctx,  // context
                      const void *in,
                      size_t inlen);  // data to be hashed

  // Generate the message digest (size given in init).
  //      Result placed in "out".
  void blake2b_final(blake2b_ctx &ctx, void *out);

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
