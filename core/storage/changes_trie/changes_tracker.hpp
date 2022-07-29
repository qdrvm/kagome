#ifndef KAGOME_STORAGE_CHANGES_TRIE_CHANGES_TRACKER
#define KAGOME_STORAGE_CHANGES_TRIE_CHANGES_TRACKER

#include "common/buffer.hpp"
#include "primitives/common.hpp"
#include "primitives/extrinsic.hpp"
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

    /**
     * Supposed to be called when a block execution starts
     */
    virtual outcome::result<void> onBlockExecutionStart(
        primitives::BlockHash new_parent_hash,
        primitives::BlockNumber new_parent_number) = 0;

    /**
     * Supposed to be called when an entry is put into the tracked storage
     * @arg new_entry states whether the entry is new, or just an update of a
     * present value
     */
    virtual outcome::result<void> onPut(common::BufferView extrinsic_index,
                                        const common::BufferView &key,
                                        const common::BufferView &value,
                                        bool new_entry) = 0;

    /**
     * Supposed to be called when a block is added to the block tree.
     */
    virtual void onBlockAdded(const primitives::BlockHash &hash) = 0;

    /**
     * Supposed to be called when an entry is removed from the tracked storage
     */
    virtual outcome::result<void> onRemove(common::BufferView extrinsic_index,
                                           const common::BufferView &key) = 0;
  };

}  // namespace kagome::storage::changes_trie

#endif  // KAGOME_STORAGE_CHANGES_TRIE_CHANGES_TRACKER
