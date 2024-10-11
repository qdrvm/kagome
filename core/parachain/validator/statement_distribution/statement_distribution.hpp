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
        StatementDistributionThreadPool &statements_distribution_thread_pool);

    private:
    std::optional<LocalValidatorState>
    find_active_validator_state(
        ValidatorIndex validator_index,
        const Groups &groups,
        const std::vector<runtime::CoreState> &availability_cores,
        const runtime::GroupDescriptor &group_rotation_info,
        const std::optional<runtime::ClaimQueueSnapshot> &maybe_claim_queue,
        size_t seconding_limit,
        size_t max_candidate_depth);

    void handle_peer_view_update(const libp2p::peer::PeerId &peer_id,
                          const network::View &view);

    void send_peer_messages_for_relay_parent(
        const libp2p::peer::PeerId &peer_id, const RelayHash &relay_parent);

    /**
     * @brief Sends peer messages corresponding for a given relay parent.
     *
     * @param peer_id Optional reference to the PeerId of the peer to send the
     * messages to.
     * @param relay_parent The hash of the relay parent block
     */
    std::optional<std::pair<std::vector<libp2p::peer::PeerId>,
                            network::VersionedValidatorProtocolMessage>>
    pending_statement_network_message(
        const StatementStore &statement_store,
        const RelayHash &relay_parent,
        const libp2p::peer::PeerId &peer,
        network::CollationVersion version,
        ValidatorIndex originator,
        const network::vstaging::CompactStatement &compact);

    void send_pending_cluster_statements(
        const RelayHash &relay_parent,
        const libp2p::peer::PeerId &peer_id,
        network::CollationVersion version,
        ValidatorIndex peer_validator_id,
        PerRelayParentState &relay_parent_state);

    void send_pending_grid_messages(
        const RelayHash &relay_parent,
        const libp2p::peer::PeerId &peer_id,
        network::CollationVersion version,
        ValidatorIndex peer_validator_id,
        const Groups &groups,
        PerRelayParentState &relay_parent_state);

    void share_local_statement(
        RelayParentState &per_relay_parent,
        const primitives::BlockHash &relay_parent,
        const SignedFullStatementWithPVD &statement);

    /**
     * @brief Circulates a statement to the validators group.
     * @param relay_parent The hash of the relay parent block. This is used to
     * identify the group of validators to which the statement should be sent.
     * @param statement The statement to be circulated. This is an indexed and
     * signed compact statement.
     */
    void circulate_statement(
        const RelayHash &relay_parent,
        RelayParentState &relay_parent_state,
        const IndexedAndSigned<network::vstaging::CompactStatement> &statement);

   private:
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
    std::shared_ptr<PoolHandlerReady> statements_distribution_thread_handler;
  };

}  // namespace kagome::parachain::statement_distribution
