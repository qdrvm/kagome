/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/mode.hpp"

#include <memory>

namespace kagome::blockchain {
  class BlockTree;
}  // namespace kagome::blockchain

namespace kagome::application::mode {
  /**
   * Prints chain info JSON.
   * Example:
   *   {
   *     "genesis_hash": "0x91...",
   *     "finalized_hash": "0x46...",
   *     "finalized_number": 100,
   *     "best_hash": "0x75..",
   *     "best_number": 105
   *   }
   */
  class PrintChainInfoMode final : public Mode {
   public:
    PrintChainInfoMode(std::shared_ptr<blockchain::BlockTree> block_tree)
        : block_tree_(std::move(block_tree)) {}

    int run() const override;

   private:
    std::shared_ptr<blockchain::BlockTree> block_tree_;
  };
}  // namespace kagome::application::mode
