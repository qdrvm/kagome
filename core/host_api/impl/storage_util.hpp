/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/types.hpp"
#include "storage/trie/types.hpp"

namespace kagome::host_api::detail {

  /**
   * Translate integer representation of state version constant into enum type
   */
  [[nodiscard]] kagome::storage::trie::StateVersion toStateVersion(
      kagome::runtime::WasmI32 state_version_int);

}  // namespace kagome::host_api::detail
