/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstddef>

namespace kagome::crypto::bip39 {
  constexpr size_t kWordBits = 11;
  constexpr size_t kDictionaryWords = 1 << kWordBits;
}  // namespace kagome::crypto::bip39
