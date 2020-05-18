#ifndef KAGOME_STORAGE_CHANGES_TRIE_STORAGE_CHANGES_TRACKER_IMPL
#define KAGOME_STORAGE_CHANGES_TRIE_STORAGE_CHANGES_TRACKER_IMPL

#include "storage/changes_trie/changes_tracker.hpp"

namespace kagome::storage::changes_trie {

  class StorageChangesTrackerImpl : public ChangesTracker {
   public:
    enum class Error { EXTRINSIC_IDX_GETTER_UNINITIALIZED = 1 };

    /**
     * Functor that returns the current extrinsic index, which is supposed to
     * be stored in the state trie
     */
    ~StorageChangesTrackerImpl() override = default;

    void setConfig(const ChangesTrieConfig &conf) override;

    void setExtrinsicIdxGetter(GetExtrinsicIndexDelegate f) override;

    outcome::result<void> onBlockChange(
        const primitives::BlockHash &key) override;

    outcome::result<void> onChange(const common::Buffer &key) override;

    outcome::result<common::Hash256> constructChangesTrie() override;

   private:
    std::map<common::Buffer,
             std::tuple<primitives::BlockHash,
                        std::vector<primitives::ExtrinsicIndex>>>
        changes_;
    primitives::BlockHash current_block_hash_;
    GetExtrinsicIndexDelegate get_extrinsic_index_;
  };

}  // namespace kagome::storage::changes_trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::changes_trie,
                          StorageChangesTrackerImpl::Error);

#endif  // KAGOME_STORAGE_CHANGES_TRIE_STORAGE_CHANGES_TRACKER_IMPL
