/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/timeline/types.hpp"
#include "primitives/block.hpp"
#include "primitives/block_header.hpp"

namespace kagome::consensus {

  enum class ValidatorStatus {
    NonValidator = 0,
    DisabledValidator,
    Validator,
    SingleValidator,
  };

  using AuthorityIndex = uint32_t;

  /// Consensus responsible for choice slot leaders, block production, etc.
  class ProductionConsensus {
   public:
    ProductionConsensus() = default;
    virtual ~ProductionConsensus() = default;

    ProductionConsensus(ProductionConsensus &&) = delete;
    ProductionConsensus(const ProductionConsensus &) = delete;
    ProductionConsensus &operator=(ProductionConsensus &&) = delete;
    ProductionConsensus &operator=(const ProductionConsensus &) = delete;

    /// Return true if this consensus is used at start network
    virtual bool isGenesisConsensus() const = 0;

    virtual ValidatorStatus getValidatorStatus(
        const primitives::BlockInfo &parent_info,
        EpochNumber epoch_number) const = 0;

    virtual outcome::result<SlotNumber> getSlot(
        const primitives::BlockHeader &header) const = 0;

    virtual outcome::result<AuthorityIndex> getAuthority(
        const primitives::BlockHeader &header) const = 0;

    virtual outcome::result<void> processSlot(
        SlotNumber slot, const primitives::BlockInfo &parent) = 0;

    virtual outcome::result<void> validateHeader(
        const primitives::BlockHeader &block_header) const = 0;

    /// Submit the equivocation report based on two blocks of one validator
    /// produced during a single slot
    /// @arg first - hash of first equivocating block
    /// @arg second - hash of second equivocating block
    /// @return success or error
    virtual outcome::result<void> reportEquivocation(
        const primitives::BlockHash &first,
        const primitives::BlockHash &second) const = 0;

   protected:
    /// Changes epoch
    /// @param epoch epoch that switch to
    /// @param block that epoch data based on
    /// @return true if epoch successfully switched
    virtual bool changeEpoch(EpochNumber epoch,
                             const primitives::BlockInfo &block) const = 0;

    /// Check slot leadership
    /// @param block parent of block for which will produced new one if node is
    /// slot-leader
    /// @param slot for which leadership is checked
    /// @return true if node is leader of provided slot
    virtual bool checkSlotLeadership(const primitives::BlockInfo &block,
                                     SlotNumber slot) = 0;

    /// Make PreRuntime digest
    virtual outcome::result<primitives::PreRuntime> makePreDigest() const = 0;

    /// Make Seal digest
    virtual outcome::result<primitives::Seal> makeSeal(
        const primitives::Block &block) const = 0;
  };

}  // namespace kagome::consensus
