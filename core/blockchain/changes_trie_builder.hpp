/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCKCHAIN_CHANGES_TRIE_HPP
#define KAGOME_BLOCKCHAIN_CHANGES_TRIE_HPP

#include <boost/optional.hpp>

#include "blockchain/changes_trie_config.hpp"
#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/extrinsic.hpp"
#include "primitives/common.hpp"

namespace kagome::blockchain {

  class ChangesTrieBuilder {
   public:
    virtual ChangesTrieBuilder &startNewTrie(primitives::BlockHash parent,
                                     boost::optional<ChangesTrieConfig> config) = 0;

    /**
     * @param key - key which value was changed
     * @param changers - indices of extrinsics that changed the entry
     */
    virtual outcome::result<void> insertExtrinsicsChange(
        const common::Buffer &key,
        const std::vector<primitives::ExtrinsicIndex> &changers) = 0;

    // virtual outcome::result<void> insertBlocksChange() = 0;

    /**
     * Completes construction of ChangesTrie and returns its root hash
     * After this, the changes trie that have been being constructed is cleared
     */
    virtual common::Hash256 finishAndGetHash() = 0;
  };

}  // namespace kagome::blockchain

#endif  // KAGOME_BLOCKCHAIN_CHANGES_TRIE_HPP
