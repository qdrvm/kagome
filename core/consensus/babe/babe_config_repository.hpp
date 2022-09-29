/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABE_BABECONFIGREPOSITORY
#define KAGOME_CONSENSUS_BABE_BABECONFIGREPOSITORY

#include "primitives/babe_configuration.hpp"

namespace kagome::consensus::babe {

  /// Keeps actual babe configuration
  class BabeConfigRepository {
   public:
    virtual ~BabeConfigRepository() = default;

    /// Returns actual babe configuration, obtaining it from runtime if needed
    /// @return actual babe configuration
    virtual const primitives::BabeConfiguration &config() = 0;
  };

}  // namespace kagome::consensus::babe

#endif  // KAGOME_CONSENSUS_BABE_BABECONFIGREPOSITORY
