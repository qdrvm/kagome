/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <optional>

namespace kagome {
  /**
   * `clear_prefix` host api limit argument.
   */
  using ClearPrefixLimit = std::optional<uint32_t>;

  /**
   * `clear_prefix` host api result.
   * https://github.com/paritytech/polkadot-sdk/blob/e5a93fbcd4a6acec7ab83865708e5c5df3534a7b/substrate/primitives/io/src/lib.rs#L159
   */
  struct KillStorageResult {
    bool more{};
    uint32_t loops{};
    bool operator==(const KillStorageResult &other) const = default;
  };
}  // namespace kagome
