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

    void statementDistributionBackedCandidate(
        const CandidateHash &candidate_hash);

    void request_attested_candidate(const libp2p::peer::PeerId &peer,
                                    RelayParentState &relay_parent_state,
                                    const RelayHash &relay_parent,
                                    const CandidateHash &candidate_hash,
                                    GroupIndex group_index);

   private:
    struct ManifestImportSuccess {
      bool acknowledge;
      ValidatorIndex sender_index;
    };
    using ManifestImportSuccessOpt = std::optional<ManifestImportSuccess>;

    void handle_response(
        outcome::result<network::vstaging::AttestedCandidateResponse> &&r,
        const RelayHash &relay_parent,
        const CandidateHash &candidate_hash,
        GroupIndex group_index);

    std::optional<LocalValidatorState> find_active_validator_state(
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

    void send_cluster_candidate_statements(const CandidateHash &candidate_hash,
                                           const RelayHash &relay_parent);

    void apply_post_confirmation(const PostConfirmation &post_confirmation);

    void send_pending_cluster_statements(
        const RelayHash &relay_parent,
        const libp2p::peer::PeerId &peer_id,
        network::CollationVersion version,
        ValidatorIndex peer_validator_id,
        PerRelayParentState &relay_parent_state);

    void send_pending_grid_messages(const RelayHash &relay_parent,
                                    const libp2p::peer::PeerId &peer_id,
                                    network::CollationVersion version,
                                    ValidatorIndex peer_validator_id,
                                    const Groups &groups,
                                    PerRelayParentState &relay_parent_state);

    void share_local_statement(RelayParentState &per_relay_parent,
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

    outcome::result<
        std::reference_wrapper<const network::vstaging::SignedCompactStatement>>
    check_statement_signature(
        SessionIndex session_index,
        const std::vector<ValidatorId> &validators,
        const RelayHash &relay_parent,
        const network::vstaging::SignedCompactStatement &statement);

    void handle_incoming_statement(
        const libp2p::peer::PeerId &peer_id,
        const network::vstaging::StatementDistributionMessageStatement &stm);

    /// Checks whether a statement is allowed, whether the signature is
    /// accurate,
    /// and importing into the cluster tracker if successful.
    ///
    /// if successful, this returns a checked signed statement if it should be
    /// imported or otherwise an error indicating a reputational fault.
    outcome::result<std::optional<network::vstaging::SignedCompactStatement>>
    handle_cluster_statement(
        const RelayHash &relay_parent,
        ClusterTracker &cluster_tracker,
        SessionIndex session,
        const runtime::SessionInfo &session_info,
        const network::vstaging::SignedCompactStatement &statement,
        ValidatorIndex cluster_sender_index);

    outcome::result<void> handle_grid_statement(
        const RelayHash &relay_parent,
        ParachainProcessorImpl::RelayParentState &per_relay_parent,
        grid::GridTracker &grid_tracker,
        const IndexedAndSigned<network::vstaging::CompactStatement> &statement,
        ValidatorIndex grid_sender_index);

    void send_backing_fresh_statements(
        const ConfirmedCandidate &confirmed,
        const RelayHash &relay_parent,
        ParachainProcessorImpl::RelayParentState &per_relay_parent,
        const std::vector<ValidatorIndex> &group,
        const CandidateHash &candidate_hash);

    network::vstaging::StatementFilter local_knowledge_filter(
        size_t group_size,
        GroupIndex group_index,
        const CandidateHash &candidate_hash,
        const StatementStore &statement_store);

    void provide_candidate_to_grid(
        const CandidateHash &candidate_hash,
        RelayParentState &relay_parent_state,
        const ConfirmedCandidate &confirmed_candidate,
        const runtime::SessionInfo &session_info);

    void fragment_chain_update_inner(
        std::optional<std::reference_wrapper<const Hash>> active_leaf_hash,
        std::optional<std::pair<std::reference_wrapper<const Hash>,
                                ParachainId>> required_parent_info,
        std::optional<std::reference_wrapper<const HypotheticalCandidate>>
            known_hypotheticals);

    void new_confirmed_candidate_fragment_chain_updates(
        const HypotheticalCandidate &candidate);

    void new_leaf_fragment_chain_updates(const Hash &leaf_hash);

    void prospective_backed_notification_fragment_chain_updates(
        ParachainId para_id, const Hash &para_head);

    ManifestImportSuccessOpt handle_incoming_manifest_common(
        const libp2p::peer::PeerId &peer_id,
        const CandidateHash &candidate_hash,
        const RelayHash &relay_parent,
        ManifestSummary manifest_summary,
        ParachainId para_id,
        grid::ManifestKind manifest_kind);

    std::deque<network::VersionedValidatorProtocolMessage>
    post_acknowledgement_statement_messages(
        ValidatorIndex recipient,
        const RelayHash &relay_parent,
        grid::GridTracker &grid_tracker,
        const StatementStore &statement_store,
        const Groups &groups,
        GroupIndex group_index,
        const CandidateHash &candidate_hash,
        const libp2p::peer::PeerId &peer,
        network::CollationVersion version);

    // Handles BackedCandidateManifest message
    // It performs various checks and operations, and if everything is
    // successful, it sends acknowledgement and statement messages to the
    // validators group or sends a request to fetch the attested candidate.
    void handle_incoming_manifest(
        const libp2p::peer::PeerId &peer_id,
        const network::vstaging::BackedCandidateManifest &msg);

    std::deque<std::pair<std::vector<libp2p::peer::PeerId>,
                         network::VersionedValidatorProtocolMessage>>
    acknowledgement_and_statement_messages(
        const libp2p::peer::PeerId &peer,
        network::CollationVersion version,
        ValidatorIndex validator_index,
        const Groups &groups,
        ParachainProcessorImpl::RelayParentState &relay_parent_state,
        const RelayHash &relay_parent,
        GroupIndex group_index,
        const CandidateHash &candidate_hash,
        const network::vstaging::StatementFilter &local_knowledge);

    void handle_incoming_acknowledgement(
        const libp2p::peer::PeerId &peer_id,
        const network::vstaging::BackedCandidateAcknowledgement
            &acknowledgement);

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
