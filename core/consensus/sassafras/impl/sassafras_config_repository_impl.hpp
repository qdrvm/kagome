/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/sassafras/sassafras_config_repository.hpp"

#include <mutex>

#include "blockchain/indexer.hpp"
#include "consensus/sassafras/has_sassafras_consensus_digest.hpp"
#include "injector/lazy.hpp"
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

namespace kagome::consensus {
  class ConsensusSelector;
  class SlotsUtil;
}  // namespace kagome::consensus

namespace kagome::crypto {
  class Hasher;
}

namespace kagome::runtime {
  class SassafrasApi;
}

namespace kagome::storage::trie {
  class TrieStorage;
}  // namespace kagome::storage::trie

namespace kagome::consensus::sassafras {

  struct SassafrasIndexedValue {
    SCALE_TIE_ONLY(config, state, next_state_warp);

    /**
     * `NextConfigData` is rare digest, so always store recent config.
     */
    primitives::NextConfigDataV2 config;
    /**
     * Current epoch read from runtime.
     * Used at genesis and after warp sync.
     */
    std::optional<std::shared_ptr<const Epoch>> state;
    /**
     * Next epoch after warp sync, when there is no block with digest.
     */
    std::optional<std::shared_ptr<const Epoch>> next_state_warp;
    /**
     * Next epoch lazily computed from `config` and digests.
     */
    std::optional<std::shared_ptr<const Epoch>> next_state;
  };

  class SassafrasConfigRepositoryImpl final
      : public SassafrasConfigRepository,
        public std::enable_shared_from_this<SassafrasConfigRepositoryImpl> {
   public:
    enum class Error {
      NOT_FOUND = 1,
      PREVIOUS_NOT_FOUND,
    };

    SassafrasConfigRepositoryImpl(
        application::AppStateManager &app_state_manager,
        std::shared_ptr<storage::SpacedStorage> persistent_storage,
        const application::AppConfiguration &app_config,
        EpochTimings &timings,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<blockchain::BlockHeaderRepository> header_repo,
        LazySPtr<ConsensusSelector> consensus_selector,
        std::shared_ptr<runtime::SassafrasApi> sassafras_api,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<storage::trie::TrieStorage> trie_storage,
        primitives::events::ChainSubscriptionEnginePtr chain_events_engine,
        LazySPtr<SlotsUtil> slots_util);

    bool prepare();

    // SassafrasConfigRepository

    outcome::result<std::shared_ptr<const Epoch>> config(
        const primitives::BlockInfo &parent_info,
        EpochNumber epoch_number) const override;

    void warp(const primitives::BlockInfo &block) override;

   private:
    outcome::result<SlotNumber> getFirstBlockSlotNumber(
        const primitives::BlockInfo &parent_info) const;

    outcome::result<std::shared_ptr<const Epoch>> config(
        const primitives::BlockInfo &block, bool next_epoch) const;

    std::shared_ptr<Epoch> applyDigests(
        const primitives::NextConfigDataV2 &config,
        const HasSassafrasConsensusDigest &digests) const;

    outcome::result<void> load(
        const primitives::BlockInfo &block,
        blockchain::Indexed<SassafrasIndexedValue> &item) const;

    outcome::result<std::shared_ptr<const Epoch>> loadPrev(
        const std::optional<primitives::BlockInfo> &prev) const;

    void warp(std::unique_lock<std::mutex> &lock,
              const primitives::BlockInfo &block);

    std::shared_ptr<storage::BufferStorage> persistent_storage_;
    bool config_warp_sync_;
    EpochTimings &timings_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    mutable std::mutex indexer_mutex_;
    mutable blockchain::Indexer<SassafrasIndexedValue> indexer_;
    std::shared_ptr<blockchain::BlockHeaderRepository> header_repo_;
    LazySPtr<ConsensusSelector> consensus_selector_;
    std::shared_ptr<runtime::SassafrasApi> sassafras_api_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<storage::trie::TrieStorage> trie_storage_;
    std::shared_ptr<primitives::events::ChainEventSubscriber> chain_sub_;
    LazySPtr<SlotsUtil> slots_util_;

    mutable std::optional<SlotNumber> first_block_slot_number_;

    log::Logger logger_;
  };

}  // namespace kagome::consensus::sassafras

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus::sassafras,
                          SassafrasConfigRepositoryImpl::Error)
