#include "storage/changes_trie/impl/storage_changes_tracker_impl.hpp"

namespace kagome::storage::changes_trie {

  StorageChangesTrackerImpl::StorageChangesTrackerImpl(
      std::shared_ptr<TrieDb> storage) {

  }

  void StorageChangesTrackerImpl::setConfig(const ChangesTrieConfig &conf) {

  }

  void StorageChangesTrackerImpl::onChange(const common::Buffer &key,
                                           ChangeInfo change_info) {

  }

}  // namespace kagome::storage::changes_trie
