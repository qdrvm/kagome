/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_FINALITYCONSENSUS
#define KAGOME_CONSENSUS_FINALITYCONSENSUS

#include "primitives/common.hpp"

namespace kagome::consensus {

  class FinalityConsensus {
   public:
    virtual ~FinalityConsensus() = default;

    FinalityConsensus(FinalityConsensus &&) noexcept = delete;
    FinalityConsensus(const FinalityConsensus &) = delete;
    FinalityConsensus &operator=(FinalityConsensus &&) noexcept = delete;
    FinalityConsensus &operator=(const FinalityConsensus &) = delete;
  };

}  // namespace kagome::consensus

#endif  // KAGOME_CONSENSUS_FINALITYCONSENSUS
