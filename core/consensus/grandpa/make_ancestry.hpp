/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <unordered_set>

#include "blockchain/block_tree.hpp"
#include "consensus/grandpa/structs.hpp"
#include "consensus/grandpa/voting_round_error.hpp"

namespace kagome::consensus::grandpa {
  /**
   * Make ancestry merke proof for GrandpaJustification
   */
  inline outcome::result<void> makeAncestry(
      GrandpaJustification &justification,
      const blockchain::BlockTree &block_tree) {
    std::vector<primitives::BlockHeader> blocks;
    std::unordered_set<primitives::BlockInfo> seen;
    for (auto &m : justification.items) {
      auto info = m.getBlockInfo();
      while (info != justification.block_info and not seen.contains(info)) {
        if (info.number <= justification.block_info.number) {
          return VotingRoundError::CANT_MAKE_ANCESTRY;
        }
        OUTCOME_TRY(block, block_tree.getBlockHeader(info.hash));
        seen.emplace(info);
        info = *block.parentInfo();
        blocks.emplace_back(std::move(block));
      }
    }
    justification.votes_ancestries = blocks;
    return outcome::success();
  }
}  // namespace kagome::consensus::grandpa
