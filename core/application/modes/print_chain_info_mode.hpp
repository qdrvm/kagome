/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APPLICATION_PRINTCHAININFOMODE
#define KAGOME_APPLICATION_PRINTCHAININFOMODE

#include "application/mode.hpp"

#include <memory>

namespace kagome::blockchain {
  class BlockTree;
}  // namespace kagome::blockchain

namespace kagome::application::mode {
  /// Prints chain info
  class PrintChainInfoMode final : public Mode {
   public:
    PrintChainInfoMode(std::shared_ptr<blockchain::BlockTree> block_tree)
        : block_tree_(std::move(block_tree)) {}

    int run() const override;

   private:
    std::shared_ptr<blockchain::BlockTree> block_tree_;
  };
}  // namespace kagome::application::mode

#endif  // KAGOME_APPLICATION_PRINTCHAININFOMODE
