/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BABE_BLOCK_VALIDATOR_HPP
#define KAGOME_BABE_BLOCK_VALIDATOR_HPP

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "blockchain/block_tree.hpp"
#include "common/logger.hpp"
#include "consensus/validation/block_validator.hpp"

namespace kagome::consensus {
  /**
   * Validation of blocks in BABE system. Based on the algorithm described here:
   * https://research.web3.foundation/en/latest/polkadot/BABE/Babe/#2-normal-phase
   */
  class BabeBlockValidator : public BlockValidator {
   private:
    using PeerId = libp2p::peer::PeerId;

   public:
    /**
     * Create an instance of BabeBlockValidator
     * @param block_tree to be used by this instance
     * @param log to write info to
     */
    explicit BabeBlockValidator(
        std::shared_ptr<blockchain::BlockTree> block_tree,
        common::Logger log = common::createLogger("BabeBlockValidator"));

    ~BabeBlockValidator() override = default;

    enum class ValidationError { TWO_BLOCKS_IN_SLOT = 1 };

    outcome::result<void> validate(const primitives::Block &block,
                                   const PeerId &peer,
                                   BabeSlotNumber slot_number) override;

   private:
    /**
     * Check, if the peer has produced a block in this slot and memorize, if the
     * peer hasn't
     * @param peer to be checked
     * @param number of the slot
     * @return true, if the peer has not produced any blocks in this slot, false
     * otherwise
     */
    bool memorizeProducer(const PeerId &peer, BabeSlotNumber number);

    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::unordered_map<BabeSlotNumber, std::unordered_set<PeerId>>
        blocks_producers_;

    common::Logger log_;
  };
}  // namespace kagome::consensus

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus,
                          BabeBlockValidator::ValidationError)

#endif  // KAGOME_BABE_BLOCK_VALIDATOR_HPP
