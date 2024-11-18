/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"

namespace kagome::parachain {
  inline const Buffer kUmpSeparator;

  inline auto skipUmpSignals(std::span<const Buffer> messages) {
    return messages.first(std::ranges::find(messages, kUmpSeparator)
                          - messages.begin());
  }
}  // namespace kagome::parachain
