/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_PRODUCTIONCONSENSUS
#define KAGOME_CONSENSUS_PRODUCTIONCONSENSUS

#include "primitives/common.hpp"

namespace kagome::consensus {

  class ProductionConsensus {
   public:
    virtual ~ProductionConsensus() = default;

    ProductionConsensus(ProductionConsensus &&) noexcept = delete;
    ProductionConsensus(const ProductionConsensus &) = delete;
    ProductionConsensus &operator=(ProductionConsensus &&) noexcept = delete;
    ProductionConsensus &operator=(const ProductionConsensus &) = delete;
  };

}  // namespace kagome::consensus

#endif  // KAGOME_CONSENSUS_PRODUCTIONCONSENSUS
