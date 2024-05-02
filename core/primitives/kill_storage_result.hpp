/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "scale/tie.hpp"

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
    SCALE_TIE(2);

    bool more{};
    uint32_t loops{};
  };
}  // namespace kagome
