#ifndef KAGOME_STORAGE_CHANGES_TRIE_STORAGE_CHANGES_TRACKER_IMPL
#define KAGOME_STORAGE_CHANGES_TRIE_STORAGE_CHANGES_TRACKER_IMPL

#include "storage/changes_trie/changes_tracker.hpp"

namespace kagome::storage::changes_trie {

  class StorageChangesTrackerImpl : public ChangesTracker {
   public:
    typedef std::function<outcome::result<common::Buffer>()>
        GetExtrinsicIndexDelegate;

    /**
     * Functor that returns the current extrinsic index, which is supposed to
     * be stored in the state trie
     */
    StorageChangesTrackerImpl(GetExtrinsicIndexDelegate f);
    ~StorageChangesTrackerImpl() override = default;

    void setConfig(const ChangesTrieConfig &conf) override;

    outcome::result<void> onBlockChange(const primitives::BlockHash &key) override;

    outcome::result<void> onChange(const common::Buffer &key) override;

    outcome::result<void> sinkToChangesTrie(
        ChangesTrieBuilder &builder) override;

   private:
    std::map<common::Buffer,
             std::tuple<primitives::BlockHash,
                        std::vector<primitives::ExtrinsicIndex>>>
        changes_;
    primitives::BlockHash current_block_hash_;
    GetExtrinsicIndexDelegate get_extrinsic_index_;
  };

}  // namespace kagome::storage::changes_trie

#endif  // KAGOME_STORAGE_CHANGES_TRIE_STORAGE_CHANGES_TRACKER_IMPL
