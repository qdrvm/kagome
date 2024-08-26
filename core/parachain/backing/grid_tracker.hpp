/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Ref: polkadot/node/network/statement-distribution/src/v2/grid.rs
 */

#pragma once

#include <bitset>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "network/types/collator_messages_vstaging.hpp"
#include "parachain/backing/grid.hpp"
#include "parachain/groups.hpp"
#include "parachain/types.hpp"

template <>
struct std::hash<std::pair<kagome::parachain::ValidatorIndex,
                           kagome::network::vstaging::CompactStatement>> {
  size_t operator()(const std::pair<kagome::parachain::ValidatorIndex,
                                    kagome::network::vstaging::CompactStatement>
                        &value) const {
    using Hash = kagome::parachain::Hash;
    using ValidatorIndex = kagome::parachain::ValidatorIndex;

    size_t result = std::hash<ValidatorIndex>()(value.first);
    auto hash = kagome::visit_in_place(
        value.second.inner_value,
        [](const kagome::network::vstaging::Empty &) -> Hash { UNREACHABLE; },
        [](const auto &v) -> Hash { return v.hash; });

    boost::hash_combine(result, std::hash<Hash>()(hash));
    return result;
  }
};

namespace kagome::parachain::grid {

  using SessionTopologyView = Views;
  using StatementFilter = network::vstaging::StatementFilter;

  /// The kind of backed candidate manifest we should send to a remote peer.
  enum class ManifestKind {
    /// Full manifests contain information about the candidate and should be
    /// sent to peers which aren't guaranteed to have the candidate already.
    Full,

    /// Acknowledgement manifests omit information which is implicit in the
    /// candidate itself, and should be sent to peers which are guaranteed to
    /// have the
    /// candidate already.
    Acknowledgement,
  };

  /// A summary of a manifest being sent by a counterparty.
  struct ManifestSummary {
    /// The claimed parent head data hash of the candidate.
    Hash claimed_parent_hash;

    /// The claimed group index assigned to the candidate.
    GroupIndex claimed_group_index = 0;

    /// A statement filter sent alongisde the candidate, communicating
    /// knowledge.
    StatementFilter statement_knowledge;
  };

  /// The knowledge we are aware of counterparties having of manifests.
  class ReceivedManifests {
    std::unordered_map<CandidateHash, ManifestSummary> received;

    /// group -> seconded counts.
    std::unordered_map<GroupIndex, std::vector<size_t>> seconded_counts;

   public:
    std::optional<StatementFilter> candidate_statement_filter(
        const CandidateHash &candidate_hash);

    /// Attempt to import a received manifest from a counterparty.
    ///
    /// This will reject manifests which are either duplicate, conflicting,
    /// or imply an irrational amount of `Seconded` statements.
    ///
    /// This assumes that the manifest has already been checked for
    /// validity - i.e. that the bitvecs match the claimed group in size
    /// and that the manifest includes at least one `Seconded`
    /// attestation and includes enough attestations for the candidate
    /// to be backed.
    ///
    /// This also should only be invoked when we are intended to track
    /// the knowledge of this peer as determined by the [`SessionTopology`].
    outcome::result<void> import_received(
        size_t group_size,
        size_t seconding_limit,
        const CandidateHash &candidate_hash,
        const ManifestSummary &manifest_summary);

   private:
    bool updating_ensure_within_seconding_limit(
        std::unordered_map<GroupIndex, std::vector<size_t>> &seconded_counts,
        GroupIndex group_index,
        size_t group_size,
        size_t seconding_limit,
        const std::vector<bool> &fresh_seconded);
  };

  /// Knowledge that we have about a remote peer concerning a candidate, and
  /// that they have about us concerning the candidate.
  struct MutualKnowledge {
    /// Knowledge the remote peer has about the candidate, as far as we're
    /// aware. `Some` only if they have advertised, acknowledged, or requested
    /// the candidate.
    std::optional<StatementFilter> remote_knowledge;

    /// Knowledge we have indicated to the remote peer about the candidate.
    /// `Some` only if we have advertised, acknowledged, or requested the
    /// candidate from them.
    std::optional<StatementFilter> local_knowledge;

    /// Knowledge peer circulated to us, this is different from
    /// `local_knowledge` and `remote_knowledge`, through the fact that includes
    /// only statements that we received from peer while the other two, after
    /// manifest exchange part will include both what we sent to the peer and
    /// what we received from peer, see `sent_or_received_direct_statement` for
    /// more details.
    std::optional<StatementFilter> received_knowledge;
  };

  /// A utility struct for keeping track of metadata about candidates
  /// we have confirmed as having been backed.
  struct KnownBackedCandidate {
    GroupIndex group_index;
    StatementFilter local_knowledge;
    std::unordered_map<ValidatorIndex, MutualKnowledge> mutual_knowledge;

    bool has_received_manifest_from(ValidatorIndex validator);
    bool has_sent_manifest_to(ValidatorIndex validator);
    bool note_fresh_statement(
        size_t statement_index_in_group,
        const network::vstaging::StatementKind &statement_kind);
    bool is_pending_statement(
        ValidatorIndex validator,
        size_t statement_index_in_group,
        const network::vstaging::StatementKind &statement_kind);

    void manifest_sent_to(ValidatorIndex validator,
                          const StatementFilter &local_knowledge);
    void manifest_received_from(ValidatorIndex validator,
                                const StatementFilter &remote_knowledge);

    void sent_or_received_direct_statement(
        ValidatorIndex validator,
        size_t statement_index_in_group,
        const network::vstaging::StatementKind &statement_kind,
        bool received);

    std::vector<std::pair<ValidatorIndex, bool>> direct_statement_senders(
        GroupIndex group_index,
        size_t originator_index_in_group,
        const network::vstaging::StatementKind &statement_kind);
    std::vector<ValidatorIndex> direct_statement_recipients(
        GroupIndex group_index,
        size_t originator_index_in_group,
        const network::vstaging::StatementKind &statement_kind);
    std::optional<StatementFilter> pending_statements(ValidatorIndex validator);
  };

  /// A tracker of knowledge from authorities within the grid for a particular
  /// relay-parent.
  struct GridTracker {
    enum class Error {
      DISALLOWED = 1,
      MALFORMED,
      INSUFFICIENT,
      CONFLICTING,
      SECONDING_OVERFLOW
    };

    /// Attempt to import a manifest advertised by a remote peer.
    ///
    /// This checks whether the peer is allowed to send us manifests
    /// about this group at this relay-parent. This also does sanity
    /// checks on the format of the manifest and the amount of votes
    /// it contains. It assumes that the votes from disabled validators
    /// are already filtered out.
    /// It has effects on the stored state only when successful.
    ///
    /// This returns a `bool` on success, which if true indicates that an
    /// acknowledgement is to be sent in response to the received manifest. This
    /// only occurs when the candidate is already known to be confirmed and
    /// backed.
    outcome::result<bool> import_manifest(
        const SessionTopologyView &session_topology,
        const Groups &groups,
        const CandidateHash &candidate_hash,
        size_t seconding_limit,
        const ManifestSummary &manifest,
        ManifestKind kind,
        ValidatorIndex sender);

    /// Add a new backed candidate to the tracker. This yields
    /// a list of validators which we should either advertise to
    /// or signal that we know the candidate, along with the corresponding
    /// type of manifest we should send.
    std::vector<std::pair<ValidatorIndex, ManifestKind>> add_backed_candidate(
        const SessionTopologyView &session_topology,
        const CandidateHash &candidate_hash,
        GroupIndex group_index,
        const StatementFilter &local_knowledge);

    /// Note that a backed candidate has been advertised to a
    /// given validator.
    void manifest_sent_to(const Groups &groups,
                          ValidatorIndex validator_index,
                          const CandidateHash &candidate_hash,
                          const StatementFilter &local_knowledge);

    /// Returns a vector of all candidates pending manifests for the specific
    /// validator, and the type of manifest we should send.
    std::vector<std::pair<CandidateHash, ManifestKind>> pending_manifests_for(
        ValidatorIndex validator_index);

    /// Returns a statement filter indicating statements that a given peer
    /// is awaiting concerning the given candidate, constrained by the
    /// statements we have ourselves.
    std::optional<StatementFilter> pending_statements_for(
        ValidatorIndex validator_index, const CandidateHash &candidate_hash);

    /// Returns a vector of all pending statements to the validator, sorted with
    /// `Seconded` statements at the front.
    ///
    /// Statements are in the form `(Originator, Statement Kind)`.
    std::vector<std::pair<ValidatorIndex, network::vstaging::CompactStatement>>
    all_pending_statements_for(ValidatorIndex validator_index);

    /// Whether a validator can request a manifest from us.
    bool can_request(ValidatorIndex validator,
                     const CandidateHash &candidate_hash);

    /// Determine the validators which can send a statement to us by direct
    /// broadcast.
    ///
    /// Returns a list of tuples representing each potential
    /// sender(ValidatorIndex) and if the sender should already know about the
    /// statement, because we just sent it to it.
    std::vector<std::pair<ValidatorIndex, bool>> direct_statement_providers(
        const Groups &groups,
        ValidatorIndex originator,
        const network::vstaging::CompactStatement &statement);

    /// Determine the validators which can receive a statement from us by direct
    /// broadcast.
    std::vector<ValidatorIndex> direct_statement_targets(
        const Groups &groups,
        ValidatorIndex originator,
        const network::vstaging::CompactStatement &statement);

    /// Note that we have learned about a statement. This will update
    /// `pending_statements_for` for any relevant validators if actually
    /// fresh.
    void learned_fresh_statement(
        const Groups &groups,
        const SessionTopologyView &session_topology,
        ValidatorIndex originator,
        const network::vstaging::CompactStatement &statement);

    /// Note that a direct statement about a given candidate was sent to or
    /// received from the given validator.
    void sent_or_received_direct_statement(
        const Groups &groups,
        ValidatorIndex originator,
        ValidatorIndex counterparty,
        const network::vstaging::CompactStatement &statement,
        bool received);

    /// Get the advertised statement filter of a validator for a candidate.
    std::optional<StatementFilter> advertised_statements(
        ValidatorIndex validator, const CandidateHash &candidate_hash);

   private:
    std::unordered_map<ValidatorIndex, ReceivedManifests> received;
    std::unordered_map<CandidateHash, KnownBackedCandidate> confirmed_backed;
    std::unordered_map<CandidateHash,
                       std::vector<std::pair<ValidatorIndex, GroupIndex>>>
        unconfirmed;
    std::unordered_map<ValidatorIndex,
                       std::unordered_map<CandidateHash, ManifestKind>>
        pending_manifests;
    std::unordered_map<
        ValidatorIndex,
        std::unordered_set<
            std::pair<ValidatorIndex, network::vstaging::CompactStatement>>>
        pending_statements;
  };

}  // namespace kagome::parachain::grid

OUTCOME_HPP_DECLARE_ERROR(kagome::parachain::grid, GridTracker::Error);
