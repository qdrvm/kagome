#ifndef KAGOME_STORAGE_CHANGES_TRIE_STORAGE_CHANGES_TRACKER_IMPL
#define KAGOME_STORAGE_CHANGES_TRIE_STORAGE_CHANGES_TRACKER_IMPL

#include "storage/changes_trie/changes_tracker.hpp"

namespace kagome::storage::changes_trie {

  class StorageChangesTrackerImpl : public ChangesTracker {
   public:
    StorageChangesTrackerImpl();
    ~StorageChangesTrackerImpl() override = default;

    void setConfig(const ChangesTrieConfig &conf) override;

    void setBlockHash(const primitives::BlockHash &hash) override;

    void onChange(const common::Buffer &key) override;

    outcome::result<void> sinkToChangesTrie(
        ChangesTrieBuilder &builder) override;

   private:
    std::map<common::Buffer,
             std::tuple<primitives::BlockNumber,
                        std::list<primitives::ExtrinsicIndex>>>
        changes_;
    primitives::BlockHash current_block_hash_;
    primitives::BlockNumber current_block_number_;
  };

}  // namespace kagome::storage::changes_trie

#endif  // KAGOME_STORAGE_CHANGES_TRIE_STORAGE_CHANGES_TRACKER_IMPL
