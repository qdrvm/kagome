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

    /// Returns duration of slot in milliseconds
    /// @return slot duration
    virtual BabeDuration slotDuration() const = 0;

    /// Returns epoch length in number of slot
    /// @return slot duration
    virtual EpochLength epochLength() const = 0;

    /// Returns actual babe configuration
    /// @return actual babe configuration
    virtual std::shared_ptr<const primitives::BabeConfiguration> config(
        const primitives::BlockInfo &parent_block,
        consensus::EpochNumber epoch_number) = 0;
  };

}  // namespace kagome::consensus::babe

#endif  // KAGOME_CONSENSUS_BABE_BABECONFIGREPOSITORY
