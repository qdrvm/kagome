/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCKCHAIN_CHANGES_TRIE_HPP
#define KAGOME_BLOCKCHAIN_CHANGES_TRIE_HPP

#include <boost/optional.hpp>

#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/common.hpp"
#include "primitives/extrinsic.hpp"
#include "storage/changes_trie/changes_trie_config.hpp"

namespace kagome::storage::changes_trie {

  /**
   * A Builder for a changes trie
   * As the trie itself is not required for now, only its root hash,
   * after construction returns this hash
   */
  class ChangesTrieBuilder {
   public:
    virtual outcome::result<std::reference_wrapper<ChangesTrieBuilder>>
    startNewTrie(const primitives::BlockHash &parent) = 0;

    /**
     * @note config is retrieved from the storage (under key ':changes_trie')
     * @returns ChangesTrieConfig - structure that describes how block mappings
     * should be created
     */
    using OptChangesTrieConfig = boost::optional<ChangesTrieConfig>;
    virtual outcome::result<OptChangesTrieConfig> getConfig() const = 0;

    /**
     * @param key - key which value was changed
     * @param changers - indices of extrinsics that changed the entry
     */
    virtual outcome::result<void> insertExtrinsicsChange(
        const common::Buffer &key,
        const std::vector<primitives::ExtrinsicIndex> &changers) = 0;

    /*
     * TODO(Harrm): Implement it once spec is in master
     * virtual outcome::result<void> insertBlocksChange(
        const common::Buffer &key,
        const std::vector<primitives::BlockNumber> &changers) = 0;*/

    /**
     * Completes construction of ChangesTrie and returns its root hash
     * After this, the changes trie that have been being constructed is cleared
     * @returns hash of the changes trie been constructed, hash of zeroes if no
     * trie was started
     */
    virtual common::Hash256 finishAndGetHash() = 0;
  };

}  // namespace kagome::storage::changes_trie

#endif  // KAGOME_BLOCKCHAIN_CHANGES_TRIE_HPP
