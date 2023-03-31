#include "storage/changes_trie/impl/storage_changes_tracker_impl.hpp"

#include "storage/predefined_keys.hpp"

namespace kagome::storage::changes_trie {
  void StorageChangesTrackerImpl::onBlockAdded(
      const primitives::BlockHash &hash,
      const primitives::events::StorageSubscriptionEnginePtr
          &storage_sub_engine,
      const primitives::events::ChainSubscriptionEnginePtr &chain_sub_engine) {
    if (actual_val_.find(storage::kRuntimeCodeKey) != actual_val_.cend()) {
      chain_sub_engine->notify(primitives::events::ChainEventType::kNewRuntime,
                               hash);
    }
    for (auto &pair : actual_val_) {
      if (pair.second) {
        SL_TRACE(logger_, "Key: {:l}; Value {:l};", pair.first, *pair.second);
      } else {
        SL_TRACE(logger_, "Key: {:l}; Removed;", pair.first);
      }
      storage_sub_engine->notify(pair.first, pair.second, hash);
    }
  }

  void StorageChangesTrackerImpl::onPut(const common::BufferView &key,
                                        const common::BufferView &value,
                                        bool is_new_entry) {
    auto it = actual_val_.find(key);
    if (it != actual_val_.end()) {
      it->second.emplace(value);
    } else {
      actual_val_.emplace(key, value);
      if (is_new_entry) {
        new_entries_.insert(common::Buffer{key});
      }
    }
  }

  void StorageChangesTrackerImpl::onRemove(const common::BufferView &key) {
    if (auto it = actual_val_.find(key); it != actual_val_.end()) {
      if (new_entries_.erase(it->first) != 0) {
        actual_val_.erase(it);
      } else {
        it->second.reset();
      }
    }
  }
}  // namespace kagome::storage::changes_trie
