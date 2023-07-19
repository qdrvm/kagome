/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABE_BABECONFIGREPOSITORYIMPL
#define KAGOME_CONSENSUS_BABE_BABECONFIGREPOSITORYIMPL

#include "consensus/babe/babe_config_repository.hpp"
#include "consensus/babe/babe_util.hpp"

#include <mutex>

#include "blockchain/indexer.hpp"
#include "consensus/babe/has_babe_consensus_digest.hpp"
#include "log/logger.hpp"
#include "primitives/block_data.hpp"
#include "primitives/event_types.hpp"
#include "primitives/scheduled_change.hpp"
#include "storage/spaced_storage.hpp"

namespace kagome::application {
  class AppStateManager;
  class AppConfiguration;
}  // namespace kagome::application
namespace kagome::blockchain {
  class BlockTree;
  class BlockHeaderRepository;
}  // namespace kagome::blockchain
namespace kagome::crypto {
  class Hasher;
}
namespace kagome::runtime {
  class BabeApi;
}
namespace kagome::storage::trie {
  class TrieStorage;
}  // namespace kagome::storage::trie

namespace kagome::consensus::babe {
  struct BabeIndexedValue {
    SCALE_TIE_ONLY(config, state);

    /**
     * `NextConfigData` is rare digest, so always store recent config.
     */
    primitives::NextConfigDataV1 config;
    /**
     * Current epoch read from runtime.
     * Used at genesis and after warp sync.
     */
    std::optional<std::shared_ptr<const primitives::BabeConfiguration>> state;
    /**
     * Next epoch lazily computed from `config` and digests.
    */
    std::optional<std::shared_ptr<const primitives::BabeConfiguration>>
        next_state;
  };

  class BabeConfigRepositoryImpl final
      : public BabeConfigRepository,
        public BabeUtil,
        public std::enable_shared_from_this<BabeConfigRepositoryImpl> {
   public:
    enum class Error {
      NOT_FOUND = 1,
      PREVIOUS_NOT_FOUND,
    };

    BabeConfigRepositoryImpl(
        application::AppStateManager &app_state_manager,
        std::shared_ptr<storage::SpacedStorage> persistent_storage,
        const application::AppConfiguration &app_config,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<blockchain::BlockHeaderRepository> header_repo,
        std::shared_ptr<runtime::BabeApi> babe_api,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<storage::trie::TrieStorage> trie_storage,
        primitives::events::ChainSubscriptionEnginePtr chain_events_engine,
        const BabeClock &clock);

    bool prepare();

    // BabeConfigRepository

    BabeDuration slotDuration() const override;

    EpochLength epochLength() const override;

    outcome::result<std::shared_ptr<const primitives::BabeConfiguration>>
    config(const primitives::BlockInfo &parent_info,
           EpochNumber epoch_number) const override;

    // BabeUtil

    BabeSlotNumber syncEpoch(
        std::function<std::tuple<BabeSlotNumber, bool>()> &&f) override;

    BabeSlotNumber getCurrentSlot() const override;

    BabeTimePoint slotStartTime(BabeSlotNumber slot) const override;
    BabeDuration remainToStartOfSlot(BabeSlotNumber slot) const override;
    BabeTimePoint slotFinishTime(BabeSlotNumber slot) const override;
    BabeDuration remainToFinishOfSlot(BabeSlotNumber slot) const override;

    EpochNumber slotToEpoch(BabeSlotNumber slot) const override;
    BabeSlotNumber slotInEpoch(BabeSlotNumber slot) const override;

    void warp(const primitives::BlockInfo &block) override;

   private:
    BabeSlotNumber getFirstBlockSlotNumber();

    outcome::result<std::shared_ptr<const primitives::BabeConfiguration>>
    config(const primitives::BlockInfo &block, bool next_epoch) const;

    std::shared_ptr<primitives::BabeConfiguration> applyDigests(
        const primitives::NextConfigDataV1 &config,
        const HasBabeConsensusDigest &digests) const;

    outcome::result<void> load(
        const primitives::BlockInfo &block,
        blockchain::Indexed<BabeIndexedValue> &item) const;

    outcome::result<std::shared_ptr<const primitives::BabeConfiguration>>
    loadPrev(const std::optional<primitives::BlockInfo> &prev) const;

    std::shared_ptr<storage::BufferStorage> persistent_storage_;
    bool config_warp_sync_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    mutable std::mutex indexer_mutex_;
    mutable blockchain::Indexer<BabeIndexedValue> indexer_;
    std::shared_ptr<blockchain::BlockHeaderRepository> header_repo_;
    std::shared_ptr<runtime::BabeApi> babe_api_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<storage::trie::TrieStorage> trie_storage_;
    std::shared_ptr<primitives::events::ChainEventSubscriber> chain_sub_;

    BabeDuration slot_duration_{};
    EpochLength epoch_length_{};

    const BabeClock &clock_;
    std::optional<BabeSlotNumber> first_block_slot_number_;
    bool is_first_block_finalized_ = false;

    log::Logger logger_;
  };

}  // namespace kagome::consensus::babe

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus::babe,
                          BabeConfigRepositoryImpl::Error)

#endif  // KAGOME_CONSENSUS_BABE_BABECONFIGREPOSITORYIMPL
