/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/backing/grid_tracker.hpp"

namespace kagome {

std::tuple<GroupIndex, parachain::CandidateHash, network::vstaging::StatementKind, size_t> extract_statement_and_group_info(
    const Groups &groups,
    ValidatorIndex originator,
    const network::vstaging::CompactStatement &statement
) {
    network::vstaging::StatementKind statement_kind;
    parachain::CandidateHash candidate_hash;

    visit_in_place(
        statement.inner_value,
        [&]() {

        },
        [&]() {
            
        }
    )
    if (statement == CompactStatement::Seconded) {
        statement_kind = network::vstaging::StatementKind::Seconded;
    } else {
        statement_kind = network::vstaging::StatementKind::Valid;
    }
    auto group = groups->byValidatorIndex(originator);
    if (!group) {
        throw std::runtime_error("Group not found");
    }
    auto index_in_group = groups->get(*group).value().index_of(originator);
    return std::make_tuple(*group, candidate_hash, statement_kind, index_in_group);
}

std::vector<std::pair<ValidatorIndex, CompactStatement>> decompose_statement_filter(
    Groups* groups,
    GroupIndex group_index,
    CandidateHash candidate_hash,
    network::vstaging::StatementFilter statement_filter
) {
    auto group = groups->get(group_index);
    std::vector<std::pair<ValidatorIndex, CompactStatement>> result;
    for (auto& i : statement_filter.seconded_in_group.ones()) {
        result.push_back({group[i], CompactStatement::Seconded});
    }
    for (auto& i : statement_filter.validated_in_group.ones()) {
        result.push_back({group[i], CompactStatement::Valid});
    }
    return result;
}

}

namespace kagome::parachain::grid {

    void GridTracker::import_manifest(
        SessionTopologyView* session_topology,
        Groups* groups,
        CandidateHash candidate_hash,
        size_t seconding_limit,
        ManifestSummary manifest,
        ManifestKind kind,
        ValidatorIndex sender
    ) {
        auto claimed_group_index = manifest.claimed_group_index;
        auto group_topology = session_topology->group_views[manifest.claimed_group_index];
        auto receiving_from = group_topology.receiving.count(sender);
        auto sending_to = group_topology.sending.count(sender);
        bool manifest_allowed;
        if (kind == ManifestKind::Full) {
            manifest_allowed = receiving_from;
        } else if (kind == ManifestKind::Acknowledgement) {
            manifest_allowed = sending_to && confirmed_backed[candidate_hash].has_sent_manifest_to(sender);
        }
        if (!manifest_allowed) {
            throw ManifestImportError::Disallowed;
        }
        auto group_size_backing_threshold = groups->get_size_and_backing_threshold(manifest.claimed_group_index);
        if (!group_size_backing_threshold) {
            throw ManifestImportError::Malformed;
        }
        auto [group_size, backing_threshold] = *group_size_backing_threshold;
        auto remote_knowledge = manifest.statement_knowledge;
        if (remote_knowledge.size() != group_size) {
            throw ManifestImportError::Malformed;
        }
        if (!remote_knowledge.has_seconded()) {
            throw ManifestImportError::Malformed;
        }
        auto votes = remote_knowledge.backing_validators();
        if (votes < backing_threshold) {
            throw ManifestImportError::Insufficient;
        }
        received[sender].import_received(
            group_size,
            seconding_limit,
            candidate_hash,
            manifest
        );
        bool ack = false;
        auto confirmed = confirmed_backed.find(candidate_hash);
        if (confirmed != confirmed_backed.end()) {
            if (receiving_from && !confirmed->second.has_sent_manifest_to(sender)) {
                pending_manifests[sender][candidate_hash] = ManifestKind::Acknowledgement;
                ack = true;
            }
            confirmed->second.manifest_received_from(sender, remote_knowledge);
            auto pending_statements = confirmed->second.pending_statements(sender);
            if (pending_statements) {
                for (auto& statement : *pending_statements) {
                    pending_statements[sender].insert(statement);
                }
            }
        } else {
            unconfirmed[candidate_hash].push_back({sender, claimed_group_index});
        }
        return ack;
    }

    std::vector<std::pair<ValidatorIndex, ManifestKind>> GridTracker::add_backed_candidate(
        SessionTopologyView* session_topology,
        CandidateHash candidate_hash,
        GroupIndex group_index,
        network::vstaging::StatementFilter local_knowledge
    ) {
        auto confirmed = confirmed_backed.find(candidate_hash);
        if (confirmed != confirmed_backed.end()) {
            return {};
        }
        confirmed_backed[candidate_hash] = KnownBackedCandidate {
            group_index,
            {},
            local_knowledge
        };
        auto& c = confirmed_backed[candidate_hash];
        std::vector<std::pair<ValidatorIndex, ManifestKind>> result;
        auto unconfirmed_candidates = unconfirmed[candidate_hash];
        unconfirmed.erase(candidate_hash);
        for (auto& [v, claimed_group_index] : unconfirmed_candidates) {
            if (claimed_group_index != group_index) {
                continue;
            }
            auto statement_filter = received[v].candidate_statement_filter(candidate_hash);
            if (!statement_filter) {
                throw std::runtime_error("unconfirmed is only populated by validators who have sent manifest");
            }
            c.manifest_received_from(v, *statement_filter);
        }
        auto group_topology = session_topology->group_views[group_index];
        auto sending_group_manifests = group_topology.sending;
        auto receiving_group_manifests = group_topology.receiving;
        for (auto& v : sending_group_manifests) {
            gum::trace!(
                target: LOG_TARGET,
                validator_index = ?v,
                ?manifest_mode,
                "Preparing to send manifest/acknowledgement"
            );
            pending_manifests[v][candidate_hash] = ManifestKind::Full;
        }
        for (auto& v : receiving_group_manifests) {
            if (c.has_received_manifest_from(v)) {
                pending_manifests[v][candidate_hash] = ManifestKind::Acknowledgement;
            }
        }
        for (auto& [v, k] : pending_manifests) {
            for (auto& [c, m] : k) {
                result.push_back({v, m});
            }
        }
        return result;
    }

    void GridTracker::manifest_sent_to(
        Groups* groups,
        ValidatorIndex validator_index,
        CandidateHash candidate_hash,
        network::vstaging::StatementFilter local_knowledge
    ) {
        auto confirmed = confirmed_backed.find(candidate_hash);
        if (confirmed != confirmed_backed.end()) {
            confirmed->second.manifest_sent_to(validator_index, local_knowledge);
            auto pending_statements = confirmed->second.pending_statements(validator_index);
            if (pending_statements) {
                for (auto& statement : *pending_statements) {
                    pending_statements[validator_index].insert(statement);
                }
            }
        }
        auto pending = pending_manifests.find(validator_index);
        if (pending != pending_manifests.end()) {
            pending->second.erase(candidate_hash);
        }
    }

    std::vector<std::pair<CandidateHash, ManifestKind>> GridTracker::pending_manifests_for(
        ValidatorIndex validator_index
    ) {
        std::vector<std::pair<CandidateHash, ManifestKind>> result;
        auto pending = pending_manifests.find(validator_index);
        if (pending != pending_manifests.end()) {
            for (auto& [c, m] : pending->second) {
                result.push_back({c, m});
            }
        }
        return result;
    }

    std::optional<network::vstaging::StatementFilter> GridTracker::pending_statements_for(
        ValidatorIndex validator_index,
        CandidateHash candidate_hash
    ) {
        auto confirmed = confirmed_backed.find(candidate_hash);
        if (confirmed == confirmed_backed.end()) {
            return {};
        }
        return confirmed->second.pending_statements(validator_index);
    }

    std::vector<std::pair<ValidatorIndex, CompactStatement>> GridTracker::all_pending_statements_for(
        ValidatorIndex validator_index
    ) {
        std::vector<std::pair<ValidatorIndex, CompactStatement>> result;
        auto pending = pending_statements.find(validator_index);
        if (pending != pending_statements.end()) {
            for (auto& statement : pending->second) {
                result.push_back(statement);
            }
        }
        std::sort(result.begin(), result.end(), [](auto& a, auto& b) {
            if (a.second == CompactStatement::Seconded) {
                return true;
            } else {
                return false;
            }
        });
        return result;
    }

    bool GridTracker::can_request(ValidatorIndex validator, CandidateHash candidate_hash) {
        auto confirmed = confirmed_backed.find(candidate_hash);
        if (confirmed == confirmed_backed.end()) {
            return false;
        }
        return confirmed->second.has_sent_manifest_to(validator) && !confirmed->second.has_received_manifest_from(validator);
    }

    std::vector<std::pair<ValidatorIndex, bool>> GridTracker::direct_statement_providers(
        Groups* groups,
        ValidatorIndex originator,
        CompactStatement statement
    ) {
        auto [g, c_h, kind, in_group] = extract_statement_and_group_info(groups, originator, statement);
        auto confirmed = confirmed_backed.find(c_h);
        if (confirmed == confirmed_backed.end()) {
            return {};
        }
        return confirmed->second.direct_statement_senders(g, in_group, kind);
    }

    std::vector<ValidatorIndex> GridTracker::direct_statement_targets(
        Groups* groups,
        ValidatorIndex originator,
        CompactStatement statement
    ) {
        auto [g, c_h, kind, in_group] = extract_statement_and_group_info(groups, originator, statement);
        auto confirmed = confirmed_backed.find(c_h);
        if (confirmed == confirmed_backed.end()) {
            return {};
        }
        return confirmed->second.direct_statement_recipients(g, in_group, kind);
    }

    void GridTracker::learned_fresh_statement(
        Groups* groups,
        SessionTopologyView* session_topology,
        ValidatorIndex originator,
        CompactStatement statement
    ) {
        auto [g, c_h, kind, in_group] = extract_statement_and_group_info(groups, originator, statement);
        auto confirmed = confirmed_backed.find(c_h);
        if (confirmed == confirmed_backed.end()) {
            return;
        }
        if (!confirmed->second.note_fresh_statement(in_group, kind)) {
            return;
        }
        auto all_group_validators = session_topology->group_views[g].sending + session_topology->group_views[g].receiving;
        for (auto& v : all_group_validators) {
            if (confirmed->second.is_pending_statement(v, in_group, kind)) {
                pending_statements[v].insert({originator, statement});
            }
        }
    }

    void GridTracker::sent_or_received_direct_statement(
        Groups* groups,
        ValidatorIndex originator,
        ValidatorIndex counterparty,
        CompactStatement statement,
        bool received
    ) {
        auto [g, c_h, kind, in_group] = extract_statement_and_group_info(groups, originator, statement);
        auto confirmed = confirmed_backed.find(c_h);
        if (confirmed == confirmed_backed.end()) {
            return;
        }
        confirmed->second.sent_or_received_direct_statement(counterparty, in_group, kind, received);
        auto pending = pending_statements.find(counterparty);
        if (pending != pending_statements.end()) {
            pending->second.erase({originator, statement});
        }
    }

    std::optional<network::vstaging::StatementFilter> GridTracker::advertised_statements(
        ValidatorIndex validator,
        CandidateHash candidate_hash
    ) {
        auto received = received.find(validator);
        if (received == received.end()) {
            return {};
        }
        return received->second.candidate_statement_filter(candidate_hash);
    }

    KnownBackedCandidate::KnownBackedCandidate(size_t index, size_t size) : group_index(index), local_knowledge(size) {}

    bool KnownBackedCandidate::has_received_manifest_from(size_t validator) {
        auto it = mutual_knowledge.find(validator);
        return it != mutual_knowledge.end() && it->second.remote_knowledge != nullptr;
    }

    bool KnownBackedCandidate::has_sent_manifest_to(size_t validator) {
        auto it = mutual_knowledge.find(validator);
        return it != mutual_knowledge.end() && it->second.local_knowledge != nullptr;
    }

    void KnownBackedCandidate::manifest_sent_to(size_t validator, network::vstaging::StatementFilter local_knowledge) {
        MutualKnowledge& k = mutual_knowledge[validator];
        k.received_knowledge = new network::vstaging::StatementFilter(local_knowledge.seconded_in_group.size());
        k.local_knowledge = new network::vstaging::StatementFilter(local_knowledge.seconded_in_group.size());
        *k.received_knowledge = network::vstaging::StatementFilter::blank(local_knowledge.seconded_in_group.size());
        *k.local_knowledge = local_knowledge;
    }

    void KnownBackedCandidate::manifest_received_from(size_t validator, network::vstaging::StatementFilter remote_knowledge) {
        MutualKnowledge& k = mutual_knowledge[validator];
        k.remote_knowledge = new network::vstaging::StatementFilter(remote_knowledge.seconded_in_group.size());
        *k.remote_knowledge = remote_knowledge;
    }

    std::vector<std::pair<size_t, bool>> KnownBackedCandidate::direct_statement_senders(size_t group_index, size_t originator_index_in_group, StatementKind statement_kind) {
        std::vector<std::pair<size_t, bool>> senders;
        if (group_index != this->group_index) {
            return senders;
        }
        for (const auto& [v, k] : mutual_knowledge) {
            if (k.remote_knowledge != nullptr && !k.received_knowledge->contains(originator_index_in_group, statement_kind)) {
                bool local_knowledge_contains = k.local_knowledge != nullptr && k.local_knowledge->contains(originator_index_in_group, statement_kind);
                senders.push_back(std::make_pair(v, local_knowledge_contains));
            }
        }
        return senders;
    }

    std::vector<size_t> KnownBackedCandidate::direct_statement_recipients(size_t group_index, size_t originator_index_in_group, StatementKind statement_kind) {
        std::vector<size_t> recipients;
        if (group_index != this->group_index) {
            return recipients;
        }
        for (const auto& [v, k] : mutual_knowledge) {
            if (k.local_knowledge != nullptr && !k.remote_knowledge->contains(originator_index_in_group, statement_kind)) {
                recipients.push_back(v);
            }
        }
        return recipients;
    }

    bool KnownBackedCandidate::note_fresh_statement(size_t statement_index_in_group, StatementKind statement_kind) {
        bool really_fresh = !local_knowledge.contains(statement_index_in_group, statement_kind);
        local_knowledge.set(statement_index_in_group, statement_kind);
        return really_fresh;
    }

    void KnownBackedCandidate::sent_or_received_direct_statement(size_t validator, size_t statement_index_in_group, StatementKind statement_kind, bool received) {
        auto it = mutual_knowledge.find(validator);
        if (it != mutual_knowledge.end()) {
            if (it->second.remote_knowledge != nullptr && it->second.local_knowledge != nullptr) {
                it->second.remote_knowledge->set(statement_index_in_group, statement_kind);
                it->second.local_knowledge->set(statement_index_in_group, statement_kind);
            }
            if (received && it->second.received_knowledge != nullptr) {
                it->second.received_knowledge->set(statement_index_in_group, statement_kind);
            }
        }
    }

    bool KnownBackedCandidate::is_pending_statement(size_t validator, size_t statement_index_in_group, StatementKind statement_kind) {
        auto it = mutual_knowledge.find(validator);
        if (it != mutual_knowledge.end() && it->second.local_knowledge != nullptr && it->second.remote_knowledge != nullptr) {
            return !it->second.remote_knowledge->contains(statement_index_in_group, statement_kind);
        }
        return false;
    }

    StatementFilter* KnownBackedCandidate::pending_statements(size_t validator) {
        StatementFilter* full_local = &local_knowledge;
        auto it = mutual_knowledge.find(validator);
        if (it != mutual_knowledge.end() && it->second.local_knowledge != nullptr && it->second.remote_knowledge != nullptr) {
            StatementFilter* remote = it->second.remote_knowledge;
            StatementFilter* result = new StatementFilter(full_local->seconded_in_group.size());
            result->seconded_in_group = full_local->seconded_in_group & !remote->seconded_in_group;
            result->validated_in_group = full_local->validated_in_group & !remote->validated_in_group;
            return result;
        }
        return nullptr;
    }
}
