/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/backing/grid_tracker.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::parachain::grid, GridTracker::Error, e) {
  using E = kagome::parachain::grid::GridTracker::Error;
  switch (e) {
    case E::DISALLOWED:
      return "Manifest disallowed";
    case E::MALFORMED:
      return "Malformed";
    case E::INSUFFICIENT:
      return "Insufficient";
    case E::CONFLICTING:
      return "Conflicting";
    case E::OVERFLOW:
      return "Overflow";
  }
  return "Unknown parachain grid tracker error";
}

namespace kagome {

  std::optional<std::tuple<parachain::GroupIndex,
                           parachain::CandidateHash,
                           network::vstaging::StatementKind,
                           size_t>>
  extract_statement_and_group_info(
      const parachain::Groups &groups,
      parachain::ValidatorIndex originator,
      const network::vstaging::CompactStatement &statement) {
    parachain::CandidateHash candidate_hash;
    network::vstaging::StatementKind statement_kind = visit_in_place(
        statement.inner_value,
        [&](const network::vstaging::SecondedCandidateHash &) {
          return network::vstaging::StatementKind::Seconded;
        },
        [&](const network::vstaging::ValidCandidateHash &) {
          return network::vstaging::StatementKind::Valid;
        });

    auto group = groups->byValidatorIndex(originator);
    if (!group) {
      return {};
    }

    auto s = groups.get(*group).value();
    for (size_t ix = 0; ix < s.size(); ++ix) {
      if (s[ix] == originator) {
        return std::make_tuple(*group, candidate_hash, statement_kind, ix);
      }
    }
    return {};
  }

  std::vector<
      std::pair<parachain::ValidatorIndex, network::vstaging::CompactStatement>>
  decompose_statement_filter(
      const parachain::Groups &groups,
      parachain::GroupIndex group_index,
      const parachain::CandidateHash &candidate_hash,
      const network::vstaging::StatementFilter &statement_filter) {
    auto group = groups.get(group_index);
    if (!group) {
      return {};
    }

    std::vector<std::pair<parachain::ValidatorIndex,
                          network::vstaging::CompactStatement>>
        result;
    auto count_ones = [&](const network::vstaging::CompactStatement &s,
                          const scale::BitVec &bf) {
      for (size_t ix = 0; ix < bf.bits.size(); ++ix) {
        if (bf.bits[ix]) {
          const auto v = (*group)[ix];
          result.emplace_back(v, s);
        }
      }
    };

    count_ones(
        network::vstaging::SecondedCandidateHash{
            .hash = candidate_hash,
        },
        statement_filter.seconded_in_group);

    count_ones(
        network::vstaging::ValidCandidateHash{
            .hash = candidate_hash,
        },
        statement_filter.validated_in_group);

    return result;
  }

}  // namespace kagome

namespace kagome::parachain::grid {

  outcome::result<bool> GridTracker::import_manifest(
      const SessionTopologyView &session_topology,
      const Groups &groups,
      const CandidateHash &candidate_hash,
      size_t seconding_limit,
      const ManifestSummary &manifest,
      ManifestKind kind,
      ValidatorIndex sender) {
    const auto claimed_group_index = manifest.claimed_group_index;
    if (claimed_group_index >= session_topology.size()) {
      return Error::DISALLOWED;
    }

    const auto &group_topology = session_topology[manifest.claimed_group_index];
    const auto receiving_from = group_topology.receiving.contains(sender);
    const auto sending_to = group_topology.sending.contains(sender);

    bool manifest_allowed;
    if (kind == ManifestKind::Full) {
      manifest_allowed = receiving_from;
    } else if (kind == ManifestKind::Acknowledgement) {
      auto it = confirmed_backed.find(candidate_hash);
      manifest_allowed = sending_to && it != confirmed_backed.end()
                      && it->second.has_sent_manifest_to(sender);
    }
    if (!manifest_allowed) {
      return Error::DISALLOWED;
    }

    auto group_size_backing_threshold =
        groups.get_size_and_backing_threshold(manifest.claimed_group_index);
    if (!group_size_backing_threshold) {
      return Error::MALFORMED;
    }

    const auto [group_size, backing_threshold] = *group_size_backing_threshold;
    auto remote_knowledge = manifest.statement_knowledge;
    if (!remote_knowledge.has_len(group_size)) {
      return Error::MALFORMED;
    }
    if (!remote_knowledge.has_seconded()) {
      return Error::MALFORMED;
    }

    const auto votes = remote_knowledge.backing_validators();
    if (votes < backing_threshold) {
      return Error::INSUFFICIENT;
    }
    if (auto error = received[sender].import_received(
            group_size, seconding_limit, candidate_hash, manifest)) {
      return *error;
    }
    bool ack = false;
    auto confirmed = confirmed_backed.find(candidate_hash);
    if (confirmed != confirmed_backed.end()) {
      if (receiving_from && !confirmed->second.has_sent_manifest_to(sender)) {
        pending_manifests[sender][candidate_hash] =
            ManifestKind::Acknowledgement;
        ack = true;
      }
      confirmed->second.manifest_received_from(sender, remote_knowledge);
      auto ps = confirmed->second.pending_statements(sender);
      if (ps) {
        auto e = decompose_statement_filter(
            groups, claimed_group_index, candidate_hash, *ps);
        pending_statements[sender].insert(e.begin(), e.end());
      }
    } else {
      unconfirmed[candidate_hash].emplace_back(sender, claimed_group_index);
    }
    return ack;
  }

  std::vector<std::pair<ValidatorIndex, ManifestKind>>
  GridTracker::add_backed_candidate(
      const SessionTopologyView &session_topology,
      const CandidateHash &candidate_hash,
      GroupIndex group_index,
      const network::vstaging::StatementFilter &local_knowledge) {
    auto confirmed = confirmed_backed.find(candidate_hash);
    if (confirmed != confirmed_backed.end()) {
      return {};
    }

    confirmed_backed[candidate_hash] = KnownBackedCandidate{
        .group_index = group_index,
        .local_knowledge = local_knowledge,
        .mutual_knowledge = {},
    };

    auto &c = confirmed_backed[candidate_hash];
    std::vector<std::pair<ValidatorIndex, ManifestKind>> result;
    std::vector<std::pair<ValidatorIndex, GroupIndex>> unconfirmed_candidates;
    if (auto it = unconfirmed.find(candidate_hash); it != unconfirmed.end()) {
      unconfirmed_candidates = std::move(it->second);
      unconfirmed.erase(it);
    }

    for (auto &[v, claimed_group_index] : unconfirmed_candidates) {
      if (claimed_group_index != group_index) {
        continue;
      }
      auto statement_filter =
          received[v].candidate_statement_filter(candidate_hash);
      if (!statement_filter) {
        throw std::runtime_error(
            "unconfirmed is only populated by validators who have sent manifest");
      }
      c.manifest_received_from(v, *statement_filter);
    }

    if (group_index >= session_topology.size()) {
      return {};
    }

    const auto &group_topology = session_topology[group_index];
    const auto &sending_group_manifests = group_topology.sending;
    const auto &receiving_group_manifests = group_topology.receiving;

    for (const auto &v : sending_group_manifests) {
            gum::trace!(
                target: LOG_TARGET,
                validator_index = ?v,
                ?manifest_mode,
                "Preparing to send manifest/acknowledgement"
            );
            pending_manifests[v][candidate_hash] = ManifestKind::Full;
    }
    for (auto &v : receiving_group_manifests) {
      if (c.has_received_manifest_from(v)) {
        pending_manifests[v][candidate_hash] = ManifestKind::Acknowledgement;
      }
    }
    for (const auto &[v, k] : pending_manifests) {
      if (auto it = k.find(candidate_hash); it != k.end()) {
        result.emplace_back(v, it->second);
      }
    }
    return result;
  }

  void GridTracker::manifest_sent_to(
      const Groups &groups,
      ValidatorIndex validator_index,
      const CandidateHash &candidate_hash,
      const network::vstaging::StatementFilter &local_knowledge) {
    auto confirmed = confirmed_backed.find(candidate_hash);
    if (confirmed != confirmed_backed.end()) {
      confirmed->second.manifest_sent_to(validator_index, local_knowledge);
      auto ps = confirmed->second.pending_statements(validator_index);
      if (ps) {
        auto e = decompose_statement_filter(
            groups, c.group_index, candidate_hash, *ps);
        pending_statements[validator_index].insert(e.begin(), e.end());
      }
    }

    auto pending = pending_manifests.find(validator_index);
    if (pending != pending_manifests.end()) {
      pending->second.erase(candidate_hash);
    }
  }

  std::vector<std::pair<CandidateHash, ManifestKind>>
  GridTracker::pending_manifests_for(ValidatorIndex validator_index) {
    std::vector<std::pair<CandidateHash, ManifestKind>> result;
    auto pending = pending_manifests.find(validator_index);
    if (pending != pending_manifests.end()) {
      for (auto &[c, m] : pending->second) {
        result.push_back({c, m});
      }
    }
    return result;
  }

  std::optional<network::vstaging::StatementFilter>
  GridTracker::pending_statements_for(ValidatorIndex validator_index,
                                      CandidateHash candidate_hash) {
    auto confirmed = confirmed_backed.find(candidate_hash);
    if (confirmed == confirmed_backed.end()) {
      return {};
    }
    return confirmed->second.pending_statements(validator_index);
  }

  std::vector<std::pair<ValidatorIndex, CompactStatement>>
  GridTracker::all_pending_statements_for(ValidatorIndex validator_index) {
    std::vector<std::pair<ValidatorIndex, CompactStatement>> result;
    auto pending = pending_statements.find(validator_index);
    if (pending != pending_statements.end()) {
      for (auto &statement : pending->second) {
        result.push_back(statement);
      }
    }
    std::sort(result.begin(), result.end(), [](auto &a, auto &b) {
      if (a.second == CompactStatement::Seconded) {
        return true;
      } else {
        return false;
      }
    });
    return result;
  }

  bool GridTracker::can_request(ValidatorIndex validator,
                                const CandidateHash &candidate_hash) {
    auto confirmed = confirmed_backed.find(candidate_hash);
    if (confirmed == confirmed_backed.end()) {
      return false;
    }
    return confirmed->second.has_sent_manifest_to(validator)
        && !confirmed->second.has_received_manifest_from(validator);
  }

  std::vector<std::pair<ValidatorIndex, bool>>
  GridTracker::direct_statement_providers(const Groups &groups,
                                          ValidatorIndex originator,
                                          const CompactStatement &statement) {
    auto [g, c_h, kind, in_group] =
        extract_statement_and_group_info(groups, originator, statement);
    auto confirmed = confirmed_backed.find(c_h);
    if (confirmed == confirmed_backed.end()) {
      return {};
    }
    return confirmed->second.direct_statement_senders(g, in_group, kind);
  }

  std::vector<ValidatorIndex> GridTracker::direct_statement_targets(
      const Groups &groups,
      ValidatorIndex originator,
      const CompactStatement &statement) {
    auto [g, c_h, kind, in_group] =
        extract_statement_and_group_info(groups, originator, statement);
    auto confirmed = confirmed_backed.find(c_h);
    if (confirmed == confirmed_backed.end()) {
      return {};
    }
    return confirmed->second.direct_statement_recipients(g, in_group, kind);
  }

  void GridTracker::learned_fresh_statement(
      const Groups &groups,
      const SessionTopologyView &session_topology,
      ValidatorIndex originator,
      const CompactStatement &statement) {
    auto [g, c_h, kind, in_group] =
        extract_statement_and_group_info(groups, originator, statement);
    auto confirmed = confirmed_backed.find(c_h);
    if (confirmed == confirmed_backed.end()) {
      return;
    }
    if (!confirmed->second.note_fresh_statement(in_group, kind)) {
      return;
    }
    for (const auto v : session_topology[g].sending) {
      if (confirmed->second.is_pending_statement(v, in_group, kind)) {
        pending_statements[v].insert({originator, statement});
      }
    }
    for (const auto v : session_topology[g].receiving) {
      if (confirmed->second.is_pending_statement(v, in_group, kind)) {
        pending_statements[v].insert({originator, statement});
      }
    }
  }

  void GridTracker::sent_or_received_direct_statement(
      const Groups &groups,
      ValidatorIndex originator,
      ValidatorIndex counterparty,
      const CompactStatement &statement,
      bool received) {
    auto [g, c_h, kind, in_group] =
        extract_statement_and_group_info(groups, originator, statement);
    auto confirmed = confirmed_backed.find(c_h);
    if (confirmed == confirmed_backed.end()) {
      return;
    }
    confirmed->second.sent_or_received_direct_statement(
        counterparty, in_group, kind, received);
    auto pending = pending_statements.find(counterparty);
    if (pending != pending_statements.end()) {
      pending->second.erase({originator, statement});
    }
  }

  std::optional<network::vstaging::StatementFilter>
  GridTracker::advertised_statements(ValidatorIndex validator,
                                     const CandidateHash &candidate_hash) {
    auto received = received.find(validator);
    if (received == received.end()) {
      return {};
    }
    return received->second.candidate_statement_filter(candidate_hash);
  }

  bool KnownBackedCandidate::has_received_manifest_from(size_t validator) {
    auto it = mutual_knowledge.find(validator);
    return it != mutual_knowledge.end() && it->second.remote_knowledge;
  }

  bool KnownBackedCandidate::has_sent_manifest_to(size_t validator) {
    auto it = mutual_knowledge.find(validator);
    return it != mutual_knowledge.end() && it->second.local_knowledge;
  }

  void KnownBackedCandidate::manifest_sent_to(
      size_t validator,
      const network::vstaging::StatementFilter &local_knowledge) {
    MutualKnowledge &k = mutual_knowledge[validator];
    k.received_knowledge = network::vstaging::StatementFilter(
        local_knowledge.seconded_in_group.bits.size());
    k.local_knowledge = local_knowledge;
  }

  void KnownBackedCandidate::manifest_received_from(
      size_t validator,
      const network::vstaging::StatementFilter &remote_knowledge) {
    MutualKnowledge &k = mutual_knowledge[validator];
    k.remote_knowledge = remote_knowledge;
  }

  std::vector<std::pair<size_t, bool>>
  KnownBackedCandidate::direct_statement_senders(
      size_t gi,
      size_t originator_index_in_group,
      const StatementKind &statement_kind) {
    std::vector<std::pair<size_t, bool>> senders;
    if (gi != this->group_index) {
      return senders;
    }
    for (const auto &[v, k] : mutual_knowledge) {
      if (k.remote_knowledge
          && !k.received_knowledge->contains(originator_index_in_group,
                                             statement_kind)) {
        bool local_knowledge_contains =
            k.local_knowledge
            && k.local_knowledge->contains(originator_index_in_group,
                                           statement_kind);
        senders.push_back(std::make_pair(v, local_knowledge_contains));
      }
    }
    return senders;
  }

  std::vector<size_t> KnownBackedCandidate::direct_statement_recipients(
      size_t gi,
      size_t originator_index_in_group,
      const StatementKind &statement_kind) {
    std::vector<size_t> recipients;
    if (gi != this->group_index) {
      return recipients;
    }
    for (const auto &[v, k] : mutual_knowledge) {
      if (k.local_knowledge
          && !k.remote_knowledge->contains(originator_index_in_group,
                                           statement_kind)) {
        recipients.push_back(v);
      }
    }
    return recipients;
  }

  bool KnownBackedCandidate::note_fresh_statement(
      size_t statement_index_in_group, const StatementKind &statement_kind) {
    bool really_fresh =
        !local_knowledge.contains(statement_index_in_group, statement_kind);
    local_knowledge.set(statement_index_in_group, statement_kind);
    return really_fresh;
  }

  void KnownBackedCandidate::sent_or_received_direct_statement(
      size_t validator,
      size_t statement_index_in_group,
      const StatementKind &statement_kind,
      bool r) {
    auto it = mutual_knowledge.find(validator);
    if (it != mutual_knowledge.end()) {
      if (it->second.remote_knowledge && it->second.local_knowledge) {
        it->second.remote_knowledge->set(statement_index_in_group,
                                         statement_kind);
        it->second.local_knowledge->set(statement_index_in_group,
                                        statement_kind);
      }
      if (r && it->second.received_knowledge) {
        it->second.received_knowledge->set(statement_index_in_group,
                                           statement_kind);
      }
    }
  }

  bool KnownBackedCandidate::is_pending_statement(
      size_t validator,
      size_t statement_index_in_group,
      const StatementKind &statement_kind) {
    auto it = mutual_knowledge.find(validator);
    if (it != mutual_knowledge.end() && it->second.local_knowledge
        && it->second.remote_knowledge) {
      return !it->second.remote_knowledge->contains(statement_index_in_group,
                                                    statement_kind);
    }
    return false;
  }

  std::optional<network::vstaging::StatementFilter>
  KnownBackedCandidate::pending_statements(size_t validator) {
    const StatementFilter &full_local = local_knowledge;
    auto it = mutual_knowledge.find(validator);
    if (it != mutual_knowledge.end() && it->second.local_knowledge
        && it->second.remote_knowledge) {
      const StatementFilter &remote = *it->second.remote_knowledge;
      network::vstaging::StatementFilter result(
          full_local.seconded_in_group.bits.size());
      for (size_t i = 0; i < full_local.seconded_in_group.bits.size(); ++i) {
        result.seconded_in_group.bits[i] = full_local.seconded_in_group.bits[i]
                                        && !remote->seconded_in_group.bits[i];
        result.validated_in_group.bits[i] =
            full_local.validated_in_group.bits[i]
            && !remote->validated_in_group.bits[i];
      }
      return result;
    }
    return nullptr;
  }

  std::optional<network::vstaging::StatementFilter>
  ReceivedManifests::candidate_statement_filter(
      const CandidateHash &candidate_hash) {
    auto it = received.find(candidate_hash);
    if (it != received.end()) {
      return it->second
          .statement_knowledge;  // Assuming clone is similar to copy
    }
    return std::nullopt;
  }

  std::optional<GridTracker::Error> ReceivedManifests::import_received(
      size_t group_size,
      size_t seconding_limit,
      const CandidateHash &candidate_hash,
      const ManifestSummary &manifest_summary) {
    auto it = received.find(candidate_hash);
    if (it != received.end()) {
      // Entry is occupied
      auto &prev = it->second;
      if (prev.claimed_group_index != manifest_summary.claimed_group_index
          || prev.claimed_parent_hash != manifest_summary.claimed_parent_hash
          || !std::includes(
              manifest_summary.statement_knowledge.seconded_in_group.begin(),
              manifest_summary.statement_knowledge.seconded_in_group.end(),
              prev.statement_knowledge.seconded_in_group.begin(),
              prev.statement_knowledge.seconded_in_group.end())
          || !std::includes(
              manifest_summary.statement_knowledge.validated_in_group.begin(),
              manifest_summary.statement_knowledge.validated_in_group.end(),
              prev.statement_knowledge.validated_in_group.begin(),
              prev.statement_knowledge.validated_in_group.end())) {
        return GridTracker::Error::CONFLICTING;
      }
      auto fresh_seconded =
          manifest_summary.statement_knowledge.seconded_in_group;
      // Assuming |= operation is similar to merging vectors
      fresh_seconded.insert(fresh_seconded.end(),
                            prev.statement_knowledge.seconded_in_group.begin(),
                            prev.statement_knowledge.seconded_in_group.end());
      bool within_limits = updating_ensure_within_seconding_limit(
          seconded_counts,
          manifest_summary.claimed_group_index,
          group_size,
          seconding_limit,
          fresh_seconded);
      if (!within_limits) {
        return GridTracker::Error::OVERFLOW;
      }
      it->second = manifest_summary;
      return {};  // Assuming this represents Ok(())
    } else {
      // Entry is vacant
      bool within_limits = updating_ensure_within_seconding_limit(
          seconded_counts,
          manifest_summary.claimed_group_index,
          group_size,
          seconding_limit,
          manifest_summary.statement_knowledge.seconded_in_group);
      if (within_limits) {
        received[candidate_hash] = manifest_summary;
        return {};
      } else {
        return GridTracker::Error::OVERFLOW;
      }
    }
  }

  bool ReceivedManifests::updating_ensure_within_seconding_limit(
      std::unordered_map<GroupIndex, std::vector<size_t>> &seconded_counts,
      int claimed_group_index,
      size_t group_size,
      size_t seconding_limit,
      const std::vector<bool> &fresh_seconded) {
    // Implementation assumed
    return true;  // Placeholder
  }

}  // namespace kagome::parachain::grid
