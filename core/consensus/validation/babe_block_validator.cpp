/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/validation/babe_block_validator.hpp"

#include <algorithm>

#include <boost/assert.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus,
                            BabeBlockValidator::ValidationError,
                            e) {
  using E = kagome::consensus::BabeBlockValidator::ValidationError;
  switch (e) {
    case E::TWO_BLOCKS_IN_SLOT:
      return "peer tried to distribute several blocks in one slot";
  }
  return "unknown error";
}

namespace kagome::consensus {
  BabeBlockValidator::BabeBlockValidator(
      std::shared_ptr<blockchain::BlockTree> block_tree, common::Logger log)
      : block_tree_{std::move(block_tree)}, log_{std::move(log)} {
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(log_);
  }

  outcome::result<void> BabeBlockValidator::validate(
      const primitives::Block &block,
      const PeerId &peer,
      BabeSlotNumber slot_number) {
    // peer must not send two blocks in one slot
    if (!memorizeProducer(peer, slot_number)) {
      return ValidationError::TWO_BLOCKS_IN_SLOT;
    }

    return outcome::success();
  }

  bool BabeBlockValidator::memorizeProducer(const PeerId &peer,
                                            BabeSlotNumber number) {
    auto slot_it = blocks_producers_.find(number);
    if (slot_it == blocks_producers_.end()) {
      // this peer is the first producer in this slot
      blocks_producers_.insert({number, std::unordered_set<PeerId>{peer}});
      return true;
    }

    auto &slot = slot_it->second;
    auto peer_in_slot = slot.find(peer);
    if (peer_in_slot != slot.end()) {
      // this peer has already produced a block in this slot
      return false;
    }

    // OK
    slot.insert(peer);
    return true;
  }
}  // namespace kagome::consensus
