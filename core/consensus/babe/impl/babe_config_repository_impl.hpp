/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/babe/babe_config_repository.hpp"

#include <mutex>

#include "blockchain/indexer.hpp"
#include "consensus/babe/has_babe_consensus_digest.hpp"
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
  class SlotsUtil;
  class ConsensusSelector;
}  // namespace kagome::consensus

namespace kagome::runtime {
  class BabeApi;
}

namespace kagome::storage::trie {
  class TrieStorage;
}  // namespace kagome::storage::trie

namespace kagome::consensus::babe {

  struct BabeIndexedValue {
    SCALE_TIE_ONLY(config, state, next_state_warp);

    /**
     * `NextConfigData` is rare digest, so always store recent config.
     */
    primitives::NextConfigDataV1 config;
    /**
     * Current epoch read from runtime.
     * Used at genesis and after warp sync.
     */
    std::optional<std::shared_ptr<const BabeConfiguration>> state;
    /**
     * Next epoch after warp sync, when there is no block with digest.
     */
    std::optional<std::shared_ptr<const BabeConfiguration>> next_state_warp;
    /**
     * Next epoch lazily computed from `config` and digests.
     */
    std::optional<std::shared_ptr<const BabeConfiguration>> next_state;
  };

  class BabeConfigRepositoryImpl final
      : public BabeConfigRepository,
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
        EpochTimings &timings,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<blockchain::BlockHeaderRepository> header_repo,
        LazySPtr<ConsensusSelector> consensus_selector,
        std::shared_ptr<runtime::BabeApi> babe_api,
        std::shared_ptr<storage::trie::TrieStorage> trie_storage,
        primitives::events::ChainSubscriptionEnginePtr chain_events_engine,
        LazySPtr<SlotsUtil> slots_util);

    bool prepare();

    // BabeConfigRepository
    outcome::result<std::shared_ptr<const BabeConfiguration>> config(
        const primitives::BlockInfo &parent_info,
        EpochNumber epoch_number) const override;

    void warp(const primitives::BlockInfo &block) override;

   private:
    outcome::result<SlotNumber> getFirstBlockSlotNumber(
        const primitives::BlockInfo &parent_info) const;

    outcome::result<std::shared_ptr<const BabeConfiguration>> config(
        const primitives::BlockInfo &block, bool next_epoch) const;

    std::shared_ptr<BabeConfiguration> applyDigests(
        const primitives::NextConfigDataV1 &config,
        const HasBabeConsensusDigest &digests) const;

    outcome::result<void> load(
        const primitives::BlockInfo &block,
        blockchain::Indexed<BabeIndexedValue> &item) const;

    outcome::result<std::shared_ptr<const BabeConfiguration>> loadPrev(
        const std::optional<primitives::BlockInfo> &prev) const;

    void warp(std::unique_lock<std::mutex> &lock,
              const primitives::BlockInfo &block);

    std::shared_ptr<storage::BufferStorage> persistent_storage_;
    bool config_warp_sync_;
    EpochTimings &timings_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    mutable std::mutex indexer_mutex_;
    mutable blockchain::Indexer<BabeIndexedValue> indexer_;
    std::shared_ptr<blockchain::BlockHeaderRepository> header_repo_;
    LazySPtr<ConsensusSelector> consensus_selector_;
    std::shared_ptr<runtime::BabeApi> babe_api_;
    std::shared_ptr<storage::trie::TrieStorage> trie_storage_;
    primitives::events::ChainSub chain_sub_;
    LazySPtr<SlotsUtil> slots_util_;

    mutable std::optional<SlotNumber> first_block_slot_number_;

    log::Logger logger_;
  };

}  // namespace kagome::consensus::babe

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus::babe,
                          BabeConfigRepositoryImpl::Error)
