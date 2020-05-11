#include "storage/changes_trie/impl/storage_changes_tracker_impl.hpp"

namespace kagome::storage::changes_trie {

  StorageChangesTrackerImpl::StorageChangesTrackerImpl() {

  }

  void StorageChangesTrackerImpl::setBlockHash(const primitives::BlockHash &hash) {

  }

  void StorageChangesTrackerImpl::setConfig(const ChangesTrieConfig &conf) {

  }

  void StorageChangesTrackerImpl::onChange(const common::Buffer &key) {

  }

  outcome::result<void> StorageChangesTrackerImpl::sinkToChangesTrie(
      ChangesTrieBuilder &builder) {

  }

}  // namespace kagome::storage::changes_trie
