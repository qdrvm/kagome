/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/beefy/types.hpp"

namespace kagome::runtime {
  class BeefyApi {
   public:
    virtual ~BeefyApi() = default;

    /**
     * Get validator set if beefy is supported.
     */
    virtual outcome::result<std::optional<consensus::beefy::ValidatorSet>>
    validatorSet(const primitives::BlockHash &block) = 0;
  };
}  // namespace kagome::runtime
