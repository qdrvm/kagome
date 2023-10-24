/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/grandpa/authority_manager.hpp"

#include <mutex>

#include "blockchain/indexer.hpp"
#include "consensus/grandpa/has_authority_set_change.hpp"
#include "log/logger.hpp"
#include "primitives/event_types.hpp"
#include "storage/spaced_storage.hpp"

namespace kagome::application {
  class AppStateManager;
}  // namespace kagome::application

namespace kagome::blockchain {
  class BlockTree;
  class BlockHeaderRepository;
}  // namespace kagome::blockchain

namespace kagome::runtime {
  class GrandpaApi;
}  // namespace kagome::runtime

namespace kagome::consensus::grandpa {
  // TODO(turuslan): #1536, westend forced changes [1491033 2318338 2437992
  // 4356716 4357394 12691801 12719004 12734588 12735037 12735379 12735523
  // 12927502 12927596 15533486 15533936]

  // TODO(turuslan): #1536, grandpa voting during forced change

  struct GrandpaIndexedValue {
    SCALE_TIE_ONLY(next_set_id, state);

    /**
     * Set id is missing from grandpa digests.
     */
    primitives::AuthoritySetId next_set_id;
    /**
     * Current authorities read from runtime.
     * Used at genesis and after warp sync.
     */
    std::optional<std::shared_ptr<const primitives::AuthoritySet>> state;
    /**
     * Next authorities lazily computed from `set_id` and digest.
     */
    std::optional<std::shared_ptr<const primitives::AuthoritySet>> next;
  };

  class AuthorityManagerImpl final
      : public AuthorityManager,
        public std::enable_shared_from_this<AuthorityManagerImpl> {
   public:
    AuthorityManagerImpl(
        std::shared_ptr<application::AppStateManager> app_state_manager,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<runtime::GrandpaApi> grandpa_api,
        std::shared_ptr<storage::SpacedStorage> persistent_storage,
        std::shared_ptr<blockchain::BlockHeaderRepository> header_repo,
        primitives::events::ChainSubscriptionEnginePtr chain_events_engine);

    bool prepare();

    // AuthorityManager

    std::optional<std::shared_ptr<const primitives::AuthoritySet>> authorities(
        const primitives::BlockInfo &target_block,
        IsBlockFinalized finalized) const override;

    void warp(const primitives::BlockInfo &block,
              const primitives::BlockHeader &header,
              const primitives::AuthoritySet &authorities) override;

   private:
    outcome::result<std::shared_ptr<const primitives::AuthoritySet>>
    authoritiesOutcome(const primitives::BlockInfo &block, bool next) const;
    std::shared_ptr<primitives::AuthoritySet> applyDigests(
        const primitives::BlockInfo &block,
        primitives::AuthoritySetId set_id,
        const HasAuthoritySetChange &digests) const;
    outcome::result<void> load(
        const primitives::BlockInfo &block,
        blockchain::Indexed<GrandpaIndexedValue> &item) const;
    outcome::result<std::shared_ptr<const primitives::AuthoritySet>> loadPrev(
        const std::optional<primitives::BlockInfo> &prev) const;

    mutable std::mutex mutex_;

    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<runtime::GrandpaApi> grandpa_api_;
    std::shared_ptr<storage::BufferStorage> persistent_storage_;
    std::shared_ptr<blockchain::BlockHeaderRepository> header_repo_;
    std::shared_ptr<primitives::events::ChainEventSubscriber> chain_sub_;

    mutable blockchain::Indexer<GrandpaIndexedValue> indexer_;

    log::Logger logger_;
  };
}  // namespace kagome::consensus::grandpa
