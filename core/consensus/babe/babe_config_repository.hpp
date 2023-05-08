/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABE_BABECONFIGREPOSITORY
#define KAGOME_CONSENSUS_BABE_BABECONFIGREPOSITORY

#include "primitives/babe_configuration.hpp"
#include "primitives/block_data.hpp"

namespace kagome::consensus::babe {

  /// Keeps actual babe configuration
  class BabeConfigRepository {
   public:
    virtual ~BabeConfigRepository() = default;

    /// Returns the duration of a slot in milliseconds
    /// @return the duration of a slot in milliseconds
    virtual BabeDuration slotDuration() const = 0;

    /// @return the epoch length in slots
    virtual EpochLength epochLength() const = 0;

    /// Returns the actual babe configuration
    /// @return the actual babe configuration
    virtual std::optional<
        std::reference_wrapper<const primitives::BabeConfiguration>>
    config(const primitives::BlockContext &context,
           EpochNumber epoch_number) const = 0;

    virtual void readFromState(const primitives::BlockInfo &block) = 0;
  };

}  // namespace kagome::consensus::babe

#endif  // KAGOME_CONSENSUS_BABE_BABECONFIGREPOSITORY
