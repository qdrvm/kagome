/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "crypto/common.hpp"

namespace kagome::crypto::bip39 {
  namespace constants {
    constexpr size_t BIP39_SEED_LEN_512 = 64u;
  }  // namespace constants

  constexpr size_t HEX_SEED_STR_LENGTH = 66u;  // '0x' + 64 hex digits
  constexpr size_t HEX_SEED_BIT_LENGTH = 32u;

  struct Bip39Tag;
  using Bip39Seed = PrivateKey<constants::BIP39_SEED_LEN_512, Bip39Tag>;

  struct RawJunction {
    bool hard = false;
    common::Hash256 cc;
  };

  struct Bip39SeedAndJunctions {
    Bip39Seed seed;
    std::vector<RawJunction> junctions;
  };
}  // namespace kagome::crypto::bip39
