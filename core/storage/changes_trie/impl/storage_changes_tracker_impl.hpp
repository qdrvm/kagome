#ifndef KAGOME_STORAGE_CHANGES_TRIE_STORAGE_CHANGES_TRACKER_IMPL
#define KAGOME_STORAGE_CHANGES_TRIE_STORAGE_CHANGES_TRACKER_IMPL

#include "storage/changes_trie/changes_tracker.hpp"

#include "log/logger.hpp"
#include "primitives/event_types.hpp"

namespace kagome::storage::changes_trie {

  class StorageChangesTrackerImpl : public ChangesTracker {
   public:
    StorageChangesTrackerImpl(primitives::events::StorageSubscriptionEnginePtr
                                  storage_subscription_engine,
                              primitives::events::ChainSubscriptionEnginePtr
                                  chain_subscription_engine);

    ~StorageChangesTrackerImpl() override = default;

    void onBlockExecutionStart(primitives::BlockHash new_parent_hash) override;

    void onPut(const common::BufferView &key,
               const common::BufferView &value,
               bool new_entry) override;
    void onBlockAdded(const primitives::BlockHash &hash) override;
    void onRemove(const common::BufferView &key) override;

   private:
    std::set<common::Buffer, std::less<>>
        new_entries_;  // entries that do not yet exist in
                       // the underlying storage
    std::map<common::Buffer, std::optional<common::Buffer>, std::less<>>
        actual_val_;

    primitives::BlockHash parent_hash_;
    primitives::events::StorageSubscriptionEnginePtr
        storage_subscription_engine_;
    primitives::events::ChainSubscriptionEnginePtr chain_subscription_engine_;
    log::Logger logger_;
  };

}  // namespace kagome::storage::changes_trie

#endif  // KAGOME_STORAGE_CHANGES_TRIE_STORAGE_CHANGES_TRACKER_IMPL
