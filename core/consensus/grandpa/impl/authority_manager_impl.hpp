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
}  // namespace kagome::blockchain

namespace kagome::runtime {
  class GrandpaApi;
}  // namespace kagome::runtime

namespace kagome::consensus::grandpa {
  // TODO(turuslan): #1857, grandpa voting during forced change

  struct GrandpaIndexedValue {
    SCALE_TIE_ONLY(next_set_id, forced_target, state);

    /**
     * Set id is missing from grandpa digests.
     */
    primitives::AuthoritySetId next_set_id;
    std::optional<primitives::BlockNumber> forced_target;
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
    primitives::events::ChainSub chain_sub_;

    mutable blockchain::Indexer<GrandpaIndexedValue> indexer_;

    log::Logger logger_;
  };
}  // namespace kagome::consensus::grandpa
