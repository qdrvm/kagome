/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie_pruner/recover_pruner_state.hpp"

#include "blockchain/block_tree.hpp"
#include "log/logger.hpp"
#include "storage/trie_pruner/trie_pruner.hpp"

namespace kagome::storage::trie_pruner {

  outcome::result<void> recoverPrunerState(
      TriePruner &pruner, const blockchain::BlockTree &block_tree) {
    static log::Logger logger =
        log::createLogger("PrunerStateRecovery", "storage");
    auto last_pruned_block = pruner.getLastPrunedBlock();
    if (!last_pruned_block.has_value()) {
      if (block_tree.bestLeaf().number != 0) {
        SL_WARN(logger,
                "Running pruner on a non-empty non-pruned storage may lead to "
                "skipping some stored states.");
        OUTCOME_TRY(
            last_finalized,
            block_tree.getBlockHeader(block_tree.getLastFinalized().hash));

        if (auto res = pruner.restoreState(last_finalized, block_tree);
            res.has_error()) {
          SL_ERROR(logger,
                   "Failed to restore trie pruner state starting from last "
                   "finalized "
                   "block: {}",
                   res.error().message());
          return res.as_failure();
        }
      } else {
        OUTCOME_TRY(
            genesis_header,
            block_tree.getBlockHeader(block_tree.getGenesisBlockHash()));
        OUTCOME_TRY(pruner.addNewState(genesis_header.state_root,
                                       trie::StateVersion::V0));
      }
    } else {
      OUTCOME_TRY(base_block_header,
                  block_tree.getBlockHeader(last_pruned_block.value().hash));
      BOOST_ASSERT(block_tree.getLastFinalized().number
                   >= last_pruned_block.value().number);
      if (auto res = pruner.restoreState(base_block_header, block_tree);
          res.has_error()) {
        SL_WARN(logger,
                "Failed to restore trie pruner state starting from base "
                "block {}: {}",
                last_pruned_block.value(),
                res.error().message());
      }
    }
    return outcome::success();
  }

}  // namespace kagome::storage::trie_pruner