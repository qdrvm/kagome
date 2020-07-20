/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_AUTHORITIES_MANAGER_IMPL
#define KAGOME_CONSENSUS_AUTHORITIES_MANAGER_IMPL

#include "consensus/authority/authority_manager.hpp"
#include "consensus/authority/authority_update_observer.hpp"

namespace kagome::authority {
  class AuthorityManagerImpl : public AuthorityManager,
                               public AuthorityUpdateObserver {
    inline static const std::vector<primitives::ConsensusEngineId>
        known_engines{primitives::kBabeEngineId, primitives::kGrandpaEngineId};

   public:
    ~AuthorityManagerImpl() override = default;

    outcome::result<primitives::AuthorityList> authorities(
        const primitives::BlockInfo &block) override;

    outcome::result<void> onScheduledChange(
        const primitives::BlockInfo &block,
        const primitives::AuthorityList &authorities,
        primitives::BlockNumber activateAt) override;

    outcome::result<void> onForcedChange(
        const primitives::BlockInfo &block,
        const primitives::AuthorityList &authorities,
        primitives::BlockNumber activateAt) override;

    outcome::result<void> onOnDisabled(const primitives::BlockInfo &block,
                                       uint64_t authority_index) override;

    outcome::result<void> onPause(const primitives::BlockInfo &block,
                                  primitives::BlockNumber activateAt) override;

    outcome::result<void> onResume(const primitives::BlockInfo &block,
                                   primitives::BlockNumber activateAt) override;

    outcome::result<void> onConsensus(
        const primitives::ConsensusEngineId &engine_id,
        const primitives::BlockInfo &block,
        const primitives::Consensus &message) override;
  };

  enum class AuthorityUpdateObserverError { UNSUPPORTED_MESSAGE_TYPE = 1 };
}  // namespace kagome::authority

OUTCOME_HPP_DECLARE_ERROR(kagome::authority, AuthorityUpdateObserverError)

#endif  // KAGOME_CONSENSUS_AUTHORITIES_MANAGER_IMPL
