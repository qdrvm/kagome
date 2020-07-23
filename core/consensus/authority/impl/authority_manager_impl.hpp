/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_AUTHORITIES_MANAGER_IMPL
#define KAGOME_CONSENSUS_AUTHORITIES_MANAGER_IMPL

#include "consensus/authority/authority_manager.hpp"
#include "consensus/authority/authority_update_observer.hpp"
#include "consensus/grandpa/finalization_observer.hpp"

#include "consensus/authority/impl/schedule_node.hpp"

namespace kagome::authority {
  class AuthorityManagerImpl : public AuthorityManager,
                               public AuthorityUpdateObserver,
                               public consensus::grandpa::FinalizationObserver {
    inline static const std::vector<primitives::ConsensusEngineId>
        known_engines{primitives::kBabeEngineId, primitives::kGrandpaEngineId};

   public:
    ~AuthorityManagerImpl() override = default;

    outcome::result<std::shared_ptr<const primitives::AuthorityList>>
    authorities(const primitives::BlockInfo &block) override;

    outcome::result<void> onScheduledChange(
        const primitives::BlockInfo &block,
        const primitives::AuthorityList &authorities,
        primitives::BlockNumber activate_at) override;

    outcome::result<void> onForcedChange(
        const primitives::BlockInfo &block,
        const primitives::AuthorityList &authorities,
        primitives::BlockNumber activate_at) override;

    outcome::result<void> onOnDisabled(const primitives::BlockInfo &block,
                                       uint64_t authority_index) override;

    outcome::result<void> onPause(const primitives::BlockInfo &block,
                                  primitives::BlockNumber activate_at) override;

    outcome::result<void> onResume(
        const primitives::BlockInfo &block,
        primitives::BlockNumber activate_at) override;

    outcome::result<void> onConsensus(
        const primitives::ConsensusEngineId &engine_id,
        const primitives::BlockInfo &block,
        const primitives::Consensus &message) override;

    void onFinalize(const primitives::BlockInfo &block) override;

   private:
    std::shared_ptr<ScheduleNode> root_;

    /**
     * @brief Find schedule_node according to the block
     * @param block for whick find schedule node
     * @return oldest schedule_node according to the block
     */
    std::shared_ptr<ScheduleNode> getAppropriateAncestor(
        const primitives::BlockInfo &block);

    bool isDirectAncestry(const primitives::BlockInfo &ancestor,
                          const primitives::BlockInfo &descendant);
  };
}  // namespace kagome::authority

#endif  // KAGOME_CONSENSUS_AUTHORITIES_MANAGER_IMPL
