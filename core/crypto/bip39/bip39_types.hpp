/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/blob.hpp"
#include "common/buffer.hpp"

namespace kagome::crypto::bip39 {
  namespace constants {
    constexpr size_t BIP39_SEED_LEN_512 = 64u;
  }  // namespace constants

  using Bip39Seed = common::Buffer;

  struct RawJunction {
    bool hard;
    common::Hash256 cc;
  };

  struct Bip39SeedAndJunctions {
    common::Buffer seed;
    std::vector<RawJunction> junctions;

    template <typename T>
    outcome::result<T> as() const {
      return T::fromSpan(
          std::span(seed).first(std::min(seed.size(), T::size())));
    }
  };
}  // namespace kagome::crypto::bip39
