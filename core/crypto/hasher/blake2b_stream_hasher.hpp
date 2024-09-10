/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <span>
#include "common/blob.hpp"
#include "crypto/blake2/blake2b.h"

namespace kagome::crypto {

  template <size_t Outlen>
  struct Blake2b_StreamHasher final {
    static constexpr size_t kOutlen = Outlen;

    static_assert((Outlen & (Outlen - 1)) == 0, "Outlen is pow 2");
    Blake2b_StreamHasher()
        : initialized_(blake2b_init(ctx_, Outlen, nullptr, 0ull) == 0) {}

    bool update(std::span<const uint8_t> buffer) {
      if (!initialized_) {
        return false;
      }
      blake2b_update(ctx_, buffer.data(), buffer.size());
      return true;
    }

    bool get_final(common::Blob<kOutlen> &out) {
      if (!initialized_) {
        return false;
      }
      blake2b_final(ctx_, out.data());
      return true;
    }

   private:
    blake2b_ctx ctx_;
    bool initialized_{false};
  };
}  // namespace kagome::crypto
