/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLAKE2B_STREAM_HASHER_HASHER_HPP_
#define KAGOME_BLAKE2B_STREAM_HASHER_HASHER_HPP_

#include "crypto/blake2/blake2b.h"
#include "crypto/stream_hasher.hpp"

namespace kagome::crypto {
  struct Blake2b_StreamHasher final : StreamHasher {
    Blake2b_StreamHasher(size_t outlen);
    ~Blake2b_StreamHasher() override = default;

    bool update(gsl::span<const uint8_t> buffer) override;
    bool get_final(gsl::span<uint8_t> out) override;

   private:
    const size_t outlen_;
    blake2b_ctx ctx_;
    bool initialized_{false};
  };
}  // namespace kagome::crypto

#endif  // KAGOME_BLAKE2B_STREAM_HASHER_HASHER_HPP_
