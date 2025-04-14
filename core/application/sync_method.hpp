/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

namespace kagome::application {

  enum class SyncMethod : uint8_t {
    /// Full sync.
    /// Download blocks fully and execute all of them
    Full,

    /// Fast sync
    /// Download all block headers, validate them. After that download state of
    /// last finalized block ans switch to full sync
    Fast,

    /// Download all block headers, validate them. Shutdown after that. Used for
    /// debug and making light-weight snapshots
    FastWithoutState,

    /// Download blocks with significant justifications
    Warp,

    /// Download only one recent block with justification
    Unsafe,

    /// Select fastest mode by time estimation
    Auto
  };

}
