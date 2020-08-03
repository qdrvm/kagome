/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GRANDPA_FINALIZATION_OBSERVER
#define KAGOME_GRANDPA_FINALIZATION_OBSERVER

#include "outcome/outcome.hpp"
#include "primitives/common.hpp"

namespace kagome::consensus::grandpa {
  class FinalizationObserver {
   public:
    virtual ~FinalizationObserver() = default;

    /**
     * Doing something at block finalized
     * @param message
     * @return failure or nothing
     */
    virtual outcome::result<void> onFinalize(
        const primitives::BlockInfo &block) = 0;
  };
}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_GRANDPA_FINALIZATION_OBSERVER
