/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <parachain/validator/backing_implicit_view.hpp>
#include <parachain/validator/statement_distribution/per_relay_parent_state.hpp>
#include "common/ref_cache.hpp"
#include "parachain/validator/impl/candidates.hpp"
#include "utils/pool_handler_ready_make.hpp"

namespace kagome::parachain::statement_distribution {

  struct StatementDistribution
      : std::enable_shared_from_this<StatementDistribution> {
    StatementDistribution(
        std::shared_ptr<parachain::ValidatorSignerFactory> sf,
        std::shared_ptr<application::AppStateManager> app_state_manager,
        ApprovalThreadPool &approval_thread_pool)
        : per_session(RefCache<SessionIndex, PerSessionState>::create()),
          signer_factory(std::move(sf)),
          approval_thread_handler(poolHandlerReadyMake(
              this, app_state_manager, approval_thread_pool, logger_)) {}

   public:
    log::Logger logger =
        log::createLogger("StatementDistribution", "parachain");

    ImplicitView implicit_view;
    Candidates candidates;
    std::unordered_map<primitives::BlockHash, PerRelayParentState>
        per_relay_parent;
    std::shared_ptr<RefCache<SessionIndex, PerSessionState>> per_session;
    std::unordered_map<libp2p::peer::PeerId, PeerState> peers;
    std::shared_ptr<parachain::ValidatorSignerFactory> signer_factory;

    /// worker thread
    std::shared_ptr<PoolHandlerReady> approval_thread_handler;
  };

}  // namespace kagome::parachain::statement_distribution
