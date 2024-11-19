/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>

#include "common/buffer.hpp"

namespace kagome::parachain {
  /// Separator between `XCM` and `UMPSignal`.
  inline const Buffer kUmpSeparator;

  /// Utility function for skipping the ump signals.
  inline auto skipUmpSignals(std::span<const Buffer> messages) {
    return messages.first(std::ranges::find(messages, kUmpSeparator)
                          - messages.begin());
  }
}  // namespace kagome::parachain
