/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLAKE2B_STREAM_HASHER_HASHER_HPP_
#define KAGOME_BLAKE2B_STREAM_HASHER_HASHER_HPP_

#include <gsl/span>
#include "crypto/blake2/blake2b.h"

namespace kagome::crypto {

  template <size_t Outlen>
  struct Blake2b_StreamHasher final {
    static_assert((Outlen & (Outlen - 1)) == 0, "Outlen is pow 2");

    Blake2b_StreamHasher() {
      initialized_ = (0 == blake2b_init(&ctx_, Outlen, nullptr, 0ull));
    }

    bool update(gsl::span<const uint8_t> buffer) {
      if (!initialized_) {
        return false;
      }
      blake2b_update(&ctx_, buffer.data(), buffer.size());
      return true;
    }

    bool get_final(gsl::span<uint8_t> out) {
      if (!initialized_) {
        return false;
      }
      blake2b_final(&ctx_, out.data());
      return true;
    }

   private:
    blake2b_ctx ctx_;
    bool initialized_{false};
  };
}  // namespace kagome::crypto

#endif  // KAGOME_BLAKE2B_STREAM_HASHER_HASHER_HPP_
