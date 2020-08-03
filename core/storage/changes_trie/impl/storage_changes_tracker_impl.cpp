#include "storage/changes_trie/impl/storage_changes_tracker_impl.hpp"

#include "scale/scale.hpp"
#include "storage/changes_trie/impl/changes_trie.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage::changes_trie,
                            StorageChangesTrackerImpl::Error,
                            e) {
  using E = kagome::storage::changes_trie::StorageChangesTrackerImpl::Error;
  switch (e) {
    case E::EXTRINSIC_IDX_GETTER_UNINITIALIZED:
      return "The delegate that returns extrinsic index is uninitialized";
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
      SubscriptionEnginePtr subscription_engine)
      : trie_factory_(std::move(trie_factory)),
        codec_(std::move(codec)),
        parent_hash_{},
        parent_number_{std::numeric_limits<primitives::BlockNumber>::max()},
        subscription_engine_(std::move(subscription_engine)) {
    BOOST_ASSERT(trie_factory_ != nullptr);
    BOOST_ASSERT(codec_ != nullptr);
  }

  outcome::result<void> StorageChangesTrackerImpl::onBlockChange(
      primitives::BlockHash new_parent_hash,
      primitives::BlockNumber new_parent_number) {
    parent_hash_ = new_parent_hash;
    parent_number_ = new_parent_number;
    // new block -- new extrinsics
    extrinsics_changes_.clear();
    new_entries_.clear();
    return outcome::success();
  }

  void StorageChangesTrackerImpl::setExtrinsicIdxGetter(
      GetExtrinsicIndexDelegate f) {
    get_extrinsic_index_ = std::move(f);
  }

  outcome::result<void> StorageChangesTrackerImpl::onPut(
      const common::Buffer &key,
      const common::Buffer &value,
      bool is_new_entry) {
    auto change_it = extrinsics_changes_.find(key);
    OUTCOME_TRY(idx_bytes, get_extrinsic_index_());
    OUTCOME_TRY(idx, scale::decode<primitives::ExtrinsicIndex>(idx_bytes));

    // if key was already changed in the same block, just add extrinsic to
    // the changers list
    if (change_it != extrinsics_changes_.end()) {
      change_it->second.push_back(idx);
    } else {
      extrinsics_changes_.insert(std::make_pair(key, std::vector{idx}));
      if (is_new_entry) {
        new_entries_.insert(key);
      }
    }
    subscription_engine_->notify(key, value, parent_hash_);
    return outcome::success();
  }

  outcome::result<void> StorageChangesTrackerImpl::onRemove(
      const common::Buffer &key) {
    auto change_it = extrinsics_changes_.find(key);
    OUTCOME_TRY(idx_bytes, get_extrinsic_index_());
    OUTCOME_TRY(idx, scale::decode<primitives::ExtrinsicIndex>(idx_bytes));

    // if key was already changed in the same block, just add extrinsic to
    // the changers list
    if (change_it != extrinsics_changes_.end()) {
      // if new entry, i. e. it doesn't exist in the persistent storage, then
      // don't track it, because it's just temporary
      if (auto i = new_entries_.find(key); i != new_entries_.end()) {
        extrinsics_changes_.erase(change_it);
        new_entries_.erase(i);
      } else {
        change_it->second.push_back(idx);
      }

    } else {
      extrinsics_changes_.insert(std::make_pair(key, std::vector{idx}));
    }
    return outcome::success();
  }

  outcome::result<common::Hash256>
  StorageChangesTrackerImpl::constructChangesTrie(
      const primitives::BlockHash &parent, const ChangesTrieConfig &conf) {
    if (parent != parent_hash_) {
      return Error::INVALID_PARENT_HASH;
    }
    OUTCOME_TRY(
        trie,
        ChangesTrie::buildFromChanges(
            parent_number_, trie_factory_, codec_, extrinsics_changes_, conf));
    return trie->getHash();
  }

}  // namespace kagome::storage::changes_trie
