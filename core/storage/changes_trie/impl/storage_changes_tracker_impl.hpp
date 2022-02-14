#ifndef KAGOME_STORAGE_CHANGES_TRIE_STORAGE_CHANGES_TRACKER_IMPL
#define KAGOME_STORAGE_CHANGES_TRIE_STORAGE_CHANGES_TRACKER_IMPL

#include "storage/changes_trie/changes_tracker.hpp"

#include "log/logger.hpp"
#include "primitives/event_types.hpp"

namespace kagome::storage::trie {
  class Codec;
  class PolkadotTrieFactory;
}  // namespace kagome::storage::trie

namespace kagome::storage::changes_trie {

  class StorageChangesTrackerImpl : public ChangesTracker {
   public:
    enum class Error { INVALID_PARENT_HASH };

    StorageChangesTrackerImpl(
        std::shared_ptr<storage::trie::PolkadotTrieFactory> trie_factory,
        std::shared_ptr<storage::trie::Codec> codec,
        primitives::events::StorageSubscriptionEnginePtr
            storage_subscription_engine,
        primitives::events::ChainSubscriptionEnginePtr
            chain_subscription_engine);

    /**
     * Functor that returns the current extrinsic index, which is supposed to
     * be stored in the state trie
     */
    ~StorageChangesTrackerImpl() override = default;

    outcome::result<void> onBlockExecutionStart(
        primitives::BlockHash new_parent_hash,
        primitives::BlockNumber new_parent_number) override;

    outcome::result<void> onPut(common::BufferView extrinsic_index,
                                const common::BufferView &key,
                                const common::BufferView &value,
                                bool new_entry) override;
    void onBlockAdded(const primitives::BlockHash &hash) override;
    void onClearPrefix(const common::BufferView &prefix) override;
    outcome::result<void> onRemove(common::BufferView extrinsic_index,
                                   const common::BufferView &key) override;

    outcome::result<common::Hash256> constructChangesTrie(
        const primitives::BlockHash &parent,
        const ChangesTrieConfig &conf) override;

   private:
    std::shared_ptr<storage::trie::PolkadotTrieFactory> trie_factory_;
    std::shared_ptr<storage::trie::Codec> codec_;

    std::map<common::Buffer,
             std::vector<primitives::ExtrinsicIndex>,
             std::less<>>
        extrinsics_changes_;
    std::set<common::Buffer, std::less<>>
        new_entries_;  // entries that do not yet exist in
                       // the underlying storage
    std::map<common::Buffer, common::Buffer, std::less<>> actual_val_;

    primitives::BlockHash parent_hash_;
    primitives::BlockNumber parent_number_;
    primitives::events::StorageSubscriptionEnginePtr
        storage_subscription_engine_;
    primitives::events::ChainSubscriptionEnginePtr chain_subscription_engine_;
    log::Logger logger_;
  };

}  // namespace kagome::storage::changes_trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::changes_trie,
                          StorageChangesTrackerImpl::Error);

#endif  // KAGOME_STORAGE_CHANGES_TRIE_STORAGE_CHANGES_TRACKER_IMPL
