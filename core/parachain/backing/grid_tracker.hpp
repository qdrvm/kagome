/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <bitset>
#include <iostream>
#include <vector>
#include <unordered_map>

#include "parachain/types.hpp"
#include "parachain/backing/grid.hpp"

namespace kagome::parachain::grid {

using SessionTopologyView = Views;

enum class ManifestKind {
    Full,
    Acknowledgement,
};

struct ManifestSummary {
    Hash claimed_parent_hash;
    GroupIndex claimed_group_index;
    network::vstaging::StatementFilter statement_knowledge;
};

enum class ManifestImportError {
    Conflicting,
    Overflow,
    Insufficient,
    Malformed,
    Disallowed,
};

struct ReceivedManifests {
    std::unordered_map<CandidateHash, ManifestSummary> received;
    std::unordered_map<GroupIndex, std::vector<size_t>> seconded_counts;
};

struct MutualKnowledge {
    std::optional<network::vstaging::StatementFilter> remote_knowledge;
    std::optional<network::vstaging::StatementFilter> local_knowledge;
    std::optional<network::vstaging::StatementFilter> received_knowledge;
};

struct GridTracker {
    void import_manifest(
        const SessionTopologyView &session_topology,
        const Groups &groups,
        const CandidateHash &candidate_hash,
        size_t seconding_limit,
        const ManifestSummary &manifest,
        ManifestKind kind,
        ValidatorIndex sender
    );

    std::vector<std::pair<ValidatorIndex, ManifestKind>> add_backed_candidate(
        const SessionTopologyView &session_topology,
        const CandidateHash &candidate_hash,
        GroupIndex group_index,
        const network::vstaging::StatementFilter &local_knowledge
    );

    void manifest_sent_to(
        const Groups &groups,
        ValidatorIndex validator_index,
        const CandidateHash &candidate_hash,
        const network::vstaging::StatementFilter &local_knowledge
    );

    std::vector<std::pair<CandidateHash, ManifestKind>> pending_manifests_for(
        ValidatorIndex validator_index
    );

    std::optional<network::vstaging::StatementFilter> pending_statements_for(
        ValidatorIndex validator_index,
        const CandidateHash &candidate_hash
    );

    std::vector<std::pair<ValidatorIndex, CompactStatement>> all_pending_statements_for(
        ValidatorIndex validator_index
    );

    bool can_request(ValidatorIndex validator, const CandidateHash &candidate_hash);

    std::vector<std::pair<ValidatorIndex, bool>> direct_statement_providers(
        const Groups &groups,
        ValidatorIndex originator,
        const CompactStatement &statement
    );

    std::vector<ValidatorIndex> direct_statement_targets(
        const Groups &groups,
        ValidatorIndex originator,
        const CompactStatement &statement
    );

    void learned_fresh_statement(
        const Groups &groups,
        const SessionTopologyView &session_topology,
        ValidatorIndex originator,
        const CompactStatement &statement
    );

    void sent_or_received_direct_statement(
        const Groups &groups,
        ValidatorIndex originator,
        ValidatorIndex counterparty,
        const CompactStatement &statement,
        bool received
    );

    std::optional<network::vstaging::StatementFilter> advertised_statements(
        ValidatorIndex validator,
        const CandidateHash &candidate_hash
    );

    private:
    std::unordered_map<ValidatorIndex, ReceivedManifests> received;
    std::unordered_map<CandidateHash, KnownBackedCandidate> confirmed_backed;
    std::unordered_map<CandidateHash, std::vector<std::pair<ValidatorIndex, GroupIndex>>> unconfirmed;
    std::unordered_map<ValidatorIndex, std::unordered_map<CandidateHash, ManifestKind>> pending_manifests;
    std::unordered_map<ValidatorIndex, std::unordered_set<std::pair<ValidatorIndex, network::vstaging::CompactStatement>>> pending_statements;
};

struct KnownBackedCandidate {
    KnownBackedCandidate(size_t index, size_t size);

    bool has_received_manifest_from(size_t validator);
    bool has_sent_manifest_to(size_t validator);
    bool note_fresh_statement(size_t statement_index_in_group, const network::vstaging::StatementKind &statement_kind);
    bool is_pending_statement(size_t validator, size_t statement_index_in_group, const network::vstaging::StatementKind &statement_kind);

    void manifest_sent_to(size_t validator, const network::vstaging::StatementFilter &local_knowledge);
    void manifest_received_from(size_t validator, const network::vstaging::StatementFilter &remote_knowledge);

    void sent_or_received_direct_statement(size_t validator, size_t statement_index_in_group, const network::vstaging::StatementKind &statement_kind, bool received);

    std::vector<std::pair<size_t, bool>> direct_statement_senders(size_t group_index, size_t originator_index_in_group, const network::vstaging::StatementKind &statement_kind);
    std::vector<size_t> direct_statement_recipients(size_t group_index, size_t originator_index_in_group, const network::vstaging::StatementKind &statement_kind);
    std::optional<network::vstaging::StatementFilter> pending_statements(size_t validator);

private:
    size_t group_index;
    network::vstaging::StatementFilter local_knowledge;
    std::unordered_map<size_t, MutualKnowledge> mutual_knowledge;
};

}