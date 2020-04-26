#ifndef KAGOME_STORAGE_CHANGES_TRIE_STORAGE_CHANGES_TRACKER_IMPL
#define KAGOME_STORAGE_CHANGES_TRIE_STORAGE_CHANGES_TRACKER_IMPL

#include "storage/changes_trie/changes_tracker.hpp"

namespace kagome::storage::changes_trie {

  class StorageChangesTrackerImpl {
   public:
    StorageChangesTrackerImpl(std::shared_ptr<TrieDb> storage);

    void setConfig(const ChangesTrieConfig &conf) override;

    void onChange(const common::Buffer &key, ChangeInfo change_info) override;

    outcome::result<void> sinkToChangesTrie(
        ChangesTrieBuilder &builder) override;
  };

}  // namespace kagome::storage::changes_trie

#endif  // KAGOME_STORAGE_CHANGES_TRIE_STORAGE_CHANGES_TRACKER_IMPL
