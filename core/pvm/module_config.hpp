/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <type_traits>
#include "primitives/math.hpp"
#include "pvm/types.hpp"

namespace kagome::pvm {

  enum GasMeteringKind {
    /// Synchronous gas metering. This will immediately abort the execution if
    /// we run out of gas.
    Sync,
    /// Asynchronous gas metering. Has a lower performance overhead compared to
    /// synchronous gas metering,
    /// but will only periodically and asynchronously check whether we still
    /// have gas remaining while
    /// the program is running.
    ///
    /// With asynchronous gas metering the program can run slightly longer than
    /// it would otherwise,
    /// and the exact point *when* it is interrupted is not deterministic, but
    /// whether the computation
    /// as a whole finishes under a given gas limit will still be strictly
    /// enforced and deterministic.
    ///
    /// This is only a hint, and the VM might still fall back to using
    /// synchronous gas metering
    /// if asynchronous metering is not available.
    Async,
  };

  struct ModuleConfig {
    uint32_t page_size;
    Opt<GasMeteringKind> gas_metering;
    bool is_strict;
    bool step_tracing;
    bool dynamic_paging;
    uint32_t aux_data_size;
    bool allow_sbrk;
    bool cache_by_hash;

    static constexpr ModuleConfig create() {
      return ModuleConfig{
          .page_size = 0x1000,
          .gas_metering = std::nullopt,
          .is_strict = false,
          .step_tracing = false,
          .dynamic_paging = false,
          .aux_data_size = 0,
          .allow_sbrk = true,
          .cache_by_hash = false,
      };
    }
  };

}  // namespace kagome::pvm
