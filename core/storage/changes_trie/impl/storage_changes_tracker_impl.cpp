#include "storage/changes_trie/impl/storage_changes_tracker_impl.hpp"

#include "runtime/common/storage_code_provider.hpp"
#include "scale/scale.hpp"
#include "storage/predefined_keys.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage::changes_trie,
                            StorageChangesTrackerImpl::Error,
                            e) {
  using E = kagome::storage::changes_trie::StorageChangesTrackerImpl::Error;
  switch (e) {
    case E::INVALID_PARENT_HASH:
      return "The supplied parent hash doesn't match the one of the current "
             "block";
  }
  return "Unknown error";
}

namespace kagome::storage::changes_trie {

  StorageChangesTrackerImpl::StorageChangesTrackerImpl(
      std::shared_ptr<storage::trie::PolkadotTrieFactory> trie_factory,
      std::shared_ptr<storage::trie::Codec> codec,
      primitives::events::StorageSubscriptionEnginePtr
          storage_subscription_engine,
      primitives::events::ChainSubscriptionEnginePtr chain_subscription_engine)
      : trie_factory_(std::move(trie_factory)),
        codec_(std::move(codec)),
        parent_number_{std::numeric_limits<primitives::BlockNumber>::max()},
        storage_subscription_engine_(std::move(storage_subscription_engine)),
        chain_subscription_engine_(std::move(chain_subscription_engine)),
        logger_{log::createLogger("Storage Changes Tracker", "changes_trie")} {
    BOOST_ASSERT(trie_factory_ != nullptr);
    BOOST_ASSERT(codec_ != nullptr);
  }

  outcome::result<void> StorageChangesTrackerImpl::onBlockExecutionStart(
      primitives::BlockHash new_parent_hash,
      primitives::BlockNumber new_parent_number) {
    parent_hash_ = new_parent_hash;
    parent_number_ = new_parent_number;
    // new block -- new extrinsics
    actual_val_.clear();
    new_entries_.clear();
    return outcome::success();
  }

  void StorageChangesTrackerImpl::onBlockAdded(
      const primitives::BlockHash &hash) {
    if (actual_val_.find(storage::kRuntimeCodeKey) != actual_val_.cend()) {
      chain_subscription_engine_->notify(
          primitives::events::ChainEventType::kNewRuntime, hash);
    }
    for (auto &pair : actual_val_) {
      if (pair.second) {
        SL_TRACE(logger_, "Key: {:l}; Value {:l};", pair.first, *pair.second);
      } else {
        SL_TRACE(logger_, "Key: {:l}; Removed;", pair.first);
      }
      storage_subscription_engine_->notify(
          pair.first, pair.second, parent_hash_);
    }
  }

  outcome::result<void> StorageChangesTrackerImpl::onPut(
      common::BufferView extrinsic_index,
      const common::BufferView &key,
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
    return outcome::success();
  }

  outcome::result<void> StorageChangesTrackerImpl::onRemove(
      common::BufferView extrinsic_index, const common::BufferView &key) {
    if (auto it = actual_val_.find(key); it != actual_val_.end()) {
      if (new_entries_.erase(it->first) != 0) {
        actual_val_.erase(it);
      } else {
        it->second.reset();
      }
    }
    return outcome::success();
  }
}  // namespace kagome::storage::changes_trie
