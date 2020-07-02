/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BIP39_TYPES_HPP
#define KAGOME_BIP39_TYPES_HPP

#include "common/buffer.hpp"

namespace kagome::crypto::bip39 {
  namespace constants {
    constexpr size_t BIP39_SEED_LEN_512 = 64u;
  }  // namespace constants

  using Bip39Seed = common::Buffer;
}  // namespace kagome::crypto::bip39

#endif  // KAGOME_BIP39_TYPES_HPP
