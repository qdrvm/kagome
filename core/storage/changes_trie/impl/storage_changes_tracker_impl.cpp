#include "storage/changes_trie/impl/storage_changes_tracker_impl.hpp"

#include "scale/scale.hpp"

namespace kagome::storage::changes_trie {

  StorageChangesTrackerImpl::StorageChangesTrackerImpl(
      GetExtrinsicIndexDelegate f)
      : get_extrinsic_index_{std::move(f)} {}

  void StorageChangesTrackerImpl::setConfig(const ChangesTrieConfig &conf) {}

  outcome::result<void> StorageChangesTrackerImpl::onBlockChange(
      const primitives::BlockHash &key) {
    current_block_hash_ = key;
  }

  outcome::result<void> StorageChangesTrackerImpl::onChange(
      const common::Buffer &key) {
    auto change_it = changes_.find(key);
    OUTCOME_TRY(idx_bytes, get_extrinsic_index_());
    OUTCOME_TRY(idx, scale::decode<primitives::ExtrinsicIndex>(idx_bytes));

    // if key was already changed in the same block, just add extrinsic to
    // the changers list
    if (change_it != changes_.end()) {
      auto block = std::get<0>(change_it->second);
      if (current_block_hash_ == block) {
        std::get<1>(change_it->second).push_back(idx);
        return outcome::success();
      }
    }
    changes_.insert(std::make_pair(
        key, std::make_tuple(current_block_hash_, std::vector{idx})));
    return outcome::success();
  }

  outcome::result<void> StorageChangesTrackerImpl::sinkToChangesTrie(
      ChangesTrieBuilder &builder) {
    OUTCOME_TRY(builder.startNewTrie(current_block_hash_));
    for (auto &[key, change] : changes_) {
      OUTCOME_TRY(builder.insertExtrinsicsChange(key, std::get<1>(change)));
    }
  }

}  // namespace kagome::storage::changes_trie
