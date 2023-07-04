/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RECOVER_PRUNER_STATE_HPP
#define KAGOME_RECOVER_PRUNER_STATE_HPP

#include "outcome/outcome.hpp"

namespace kagome::blockchain {
  class BlockTree;
}

namespace kagome::storage::trie_pruner {

  class TriePruner;

  outcome::result<void> recoverPrunerState(
      TriePruner &pruner, const blockchain::BlockTree &block_tree);

}

#endif  // KAGOME_RECOVER_PRUNER_STATE_HPP
