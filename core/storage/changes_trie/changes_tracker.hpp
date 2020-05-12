#ifndef KAGOME_STORAGE_CHANGES_TRIE_CHANGES_TRACKER
#define KAGOME_STORAGE_CHANGES_TRIE_CHANGES_TRACKER

#include "common/buffer.hpp"
#include "primitives/extrinsic.hpp"
#include "storage/changes_trie/changes_trie_builder.hpp"
#include "storage/changes_trie/changes_trie_config.hpp"

namespace kagome::storage::changes_trie {

  /**
   * Stores the information about changes to the storage in accordance with the
   * supplied config. Used to build the changes trie.
   * onChange() must be called every time a persistent change to the node
   * storage is made.
   */
  class ChangesTracker {
   public:
    virtual ~ChangesTracker() = default;

    virtual void setConfig(const ChangesTrieConfig &conf) = 0;

    /**
     * Supposed to be called when the processed block changes
     */
    virtual outcome::result<void> onBlockChange(
        const primitives::BlockHash &key) = 0;

    /**
     * Supposed to be called when a storage entry is changed
     */
    virtual outcome::result<void> onChange(const common::Buffer &key) = 0;

    /**
     * Sinks accumulated changes for the latest registered block to the changes
     * trie builder, first calling startNewTrie on it
     */
    virtual outcome::result<void> sinkToChangesTrie(
        ChangesTrieBuilder &builder) = 0;
  };

}  // namespace kagome::storage::changes_trie

#endif  // KAGOME_STORAGE_CHANGES_TRIE_CHANGES_TRACKER
