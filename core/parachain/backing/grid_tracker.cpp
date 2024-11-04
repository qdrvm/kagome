/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/backing/grid_tracker.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::parachain::grid, GridTracker::Error, e) {
  using E = kagome::parachain::grid::GridTracker::Error;
  switch (e) {
    case E::DISALLOWED_GROUP_INDEX:
      return "Manifest disallowed group index";
    case E::DISALLOWED_DIRECTION:
      return "Manifest disallowed direction";
    case E::MALFORMED_BACKING_THRESHOLD:
      return "Malformed backing threshold";
    case E::MALFORMED_REMOTE_KNOWLEDGE_LEN:
      return "Malformed remote knowledge len";
    case E::MALFORMED_HAS_SECONDED:
      return "Malformed has seconded";
    case E::INSUFFICIENT:
      return "Insufficient";
    case E::CONFLICTING:
      return "Conflicting";
    case E::SECONDING_OVERFLOW:
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
    namespace vs = network::vstaging;

    const auto [statement_kind, candidate_hash] = visit_in_place(
        statement.inner_value,
        [&](const network::vstaging::SecondedCandidateHash &v)
            -> std::pair<vs::StatementKind,
                         std::optional<std::reference_wrapper<
                             const parachain::CandidateHash>>> {
          return {network::vstaging::StatementKind::Seconded,
                  std::cref(v.hash)};
        },
        [&](const network::vstaging::ValidCandidateHash &v)
            -> std::pair<vs::StatementKind,
                         std::optional<std::reference_wrapper<
                             const parachain::CandidateHash>>> {
          return {network::vstaging::StatementKind::Valid, std::cref(v.hash)};
        },
        [&](const auto &) -> std::pair<vs::StatementKind,
                                       std::optional<std::reference_wrapper<
                                           const parachain::CandidateHash>>> {
          return std::make_pair(network::vstaging::StatementKind::Valid,
                                std::nullopt);
        });

    if (!candidate_hash) {
      UNREACHABLE;
      return {};
    }

    auto group = groups.byValidatorIndex(originator);
    if (!group) {
      return {};
    }

    auto s = groups.get(*group).value();
    for (size_t ix = 0; ix < s.size(); ++ix) {
      if (s[ix] == originator) {
        return std::make_tuple(
            *group, candidate_hash->get(), statement_kind, ix);
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
      const parachain::grid::StatementFilter &statement_filter) {
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
      return Error::DISALLOWED_GROUP_INDEX;
    }

    const auto &group_topology = session_topology[manifest.claimed_group_index];
    const auto receiving_from = group_topology.receiving.contains(sender);
    const auto sending_to = group_topology.sending.contains(sender);

    bool manifest_allowed = true;
    if (kind == ManifestKind::Full) {
      manifest_allowed = receiving_from;
      SL_TRACE(logger,
               "Manifest full allowed. (receiving_from={})",
               manifest_allowed ? "[yes]" : "[no]");
    } else {
      auto it = confirmed_backed.find(candidate_hash);
      manifest_allowed = sending_to && it != confirmed_backed.end()
                      && it->second.has_sent_manifest_to(sender);
      SL_TRACE(logger,
               "Manifest acknowledgement allowed. (sending_to={}, "
               "has_confirmed_back={}, has_sent_manifest_to={})",
               sending_to ? "[yes]" : "[no]",
               (it != confirmed_backed.end()) ? "[yes]" : "[no]",
               (it != confirmed_backed.end()
                && it->second.has_sent_manifest_to(sender))
                   ? "[yes]"
                   : "[no]");
    }
    if (!manifest_allowed) {
      //return Error::DISALLOWED_DIRECTION;
    }

    auto group_size_backing_threshold =
        groups.get_size_and_backing_threshold(manifest.claimed_group_index);
    if (!group_size_backing_threshold) {
      return Error::MALFORMED_BACKING_THRESHOLD;
    }

    const auto [group_size, backing_threshold] = *group_size_backing_threshold;
    auto remote_knowledge = manifest.statement_knowledge;
    if (!remote_knowledge.has_len(group_size)) {
      return Error::MALFORMED_REMOTE_KNOWLEDGE_LEN;
    }
    if (!remote_knowledge.has_seconded()) {
      return Error::MALFORMED_HAS_SECONDED;
    }

    const auto votes = remote_knowledge.backing_validators();
    if (votes < backing_threshold) {
      return Error::INSUFFICIENT;
    }
    OUTCOME_TRY(received[sender].import_received(
        group_size, seconding_limit, candidate_hash, manifest));
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
  GridTracker::add_backed_candidate(const SessionTopologyView &session_topology,
                                    const CandidateHash &candidate_hash,
                                    GroupIndex group_index,
                                    const StatementFilter &local_knowledge) {
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
            "unconfirmed is only populated by validators who have sent "
            "manifest");
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

  void GridTracker::manifest_sent_to(const Groups &groups,
                                     ValidatorIndex validator_index,
                                     const CandidateHash &candidate_hash,
                                     const StatementFilter &local_knowledge) {
    auto confirmed = confirmed_backed.find(candidate_hash);
    if (confirmed != confirmed_backed.end()) {
      confirmed->second.manifest_sent_to(validator_index, local_knowledge);
      auto ps = confirmed->second.pending_statements(validator_index);
      if (ps) {
        auto e = decompose_statement_filter(
            groups, confirmed->second.group_index, candidate_hash, *ps);
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
        result.emplace_back(c, m);
      }
    }
    return result;
  }

  std::optional<StatementFilter> GridTracker::pending_statements_for(
      ValidatorIndex validator_index, const CandidateHash &candidate_hash) {
    auto confirmed = confirmed_backed.find(candidate_hash);
    if (confirmed == confirmed_backed.end()) {
      return {};
    }
    return confirmed->second.pending_statements(validator_index);
  }

  std::vector<std::pair<ValidatorIndex, network::vstaging::CompactStatement>>
  GridTracker::all_pending_statements_for(ValidatorIndex validator_index) {
    std::vector<std::pair<ValidatorIndex, network::vstaging::CompactStatement>>
        result;
    auto pending = pending_statements.find(validator_index);
    if (pending != pending_statements.end()) {
      for (auto &statement : pending->second) {
        result.push_back(statement);
      }
    }
    std::ranges::sort(result, [](auto &a, auto &b) {
      return visit_in_place(
          a.second.inner_value,
          [](const network::vstaging::SecondedCandidateHash &) { return true; },
          [](const auto &) { return false; });
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
  GridTracker::direct_statement_providers(
      const Groups &groups,
      ValidatorIndex originator,
      const network::vstaging::CompactStatement &statement) {
    auto r = extract_statement_and_group_info(groups, originator, statement);
    if (!r) {
      return {};
    }
    const auto &[g, c_h, kind, in_group] = *r;
    auto confirmed = confirmed_backed.find(c_h);
    if (confirmed == confirmed_backed.end()) {
      return {};
    }
    return confirmed->second.direct_statement_senders(g, in_group, kind);
  }

  std::vector<ValidatorIndex> GridTracker::direct_statement_targets(
      const Groups &groups,
      ValidatorIndex originator,
      const network::vstaging::CompactStatement &statement) {
    auto r = extract_statement_and_group_info(groups, originator, statement);
    if (!r) {
      return {};
    }
    const auto &[g, c_h, kind, in_group] = *r;
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
      const network::vstaging::CompactStatement &statement) {
    auto r = extract_statement_and_group_info(groups, originator, statement);
    const auto &[g, c_h, kind, in_group] = *r;

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
      const network::vstaging::CompactStatement &statement,
      bool received) {
    auto r = extract_statement_and_group_info(groups, originator, statement);
    if (!r) {
      return;
    }
    const auto &[g, c_h, kind, in_group] = *r;

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

  std::optional<StatementFilter> GridTracker::advertised_statements(
      ValidatorIndex validator, const CandidateHash &candidate_hash) {
    auto r = received.find(validator);
    if (r == received.end()) {
      return {};
    }
    return r->second.candidate_statement_filter(candidate_hash);
  }

  bool KnownBackedCandidate::has_received_manifest_from(
      ValidatorIndex validator) {
    auto it = mutual_knowledge.find(validator);
    return it != mutual_knowledge.end() && it->second.remote_knowledge;
  }

  bool KnownBackedCandidate::has_sent_manifest_to(ValidatorIndex validator) {
    auto it = mutual_knowledge.find(validator);
    return it != mutual_knowledge.end() && it->second.local_knowledge;
  }

  void KnownBackedCandidate::manifest_sent_to(
      ValidatorIndex validator, const StatementFilter &local_knowledge) {
    MutualKnowledge &k = mutual_knowledge[validator];
    k.received_knowledge =
        StatementFilter(local_knowledge.seconded_in_group.bits.size());
    k.local_knowledge = local_knowledge;
  }

  void KnownBackedCandidate::manifest_received_from(
      ValidatorIndex validator, const StatementFilter &remote_knowledge) {
    MutualKnowledge &k = mutual_knowledge[validator];
    k.remote_knowledge = remote_knowledge;
  }

  std::vector<std::pair<ValidatorIndex, bool>>
  KnownBackedCandidate::direct_statement_senders(
      GroupIndex gi,
      size_t originator_index_in_group,
      const network::vstaging::StatementKind &statement_kind) {
    std::vector<std::pair<ValidatorIndex, bool>> senders;
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
        senders.emplace_back(v, local_knowledge_contains);
      }
    }
    return senders;
  }

  std::vector<ValidatorIndex> KnownBackedCandidate::direct_statement_recipients(
      GroupIndex gi,
      size_t originator_index_in_group,
      const network::vstaging::StatementKind &statement_kind) {
    std::vector<ValidatorIndex> recipients;
    if (gi != this->group_index) {
      return recipients;
    }
    for (const auto &[v, k] : mutual_knowledge) {
      if (!k.local_knowledge) {
        continue;
      }

      if (!k.remote_knowledge) {
        continue;
      }

      if (!k.remote_knowledge->contains(originator_index_in_group,
                                        statement_kind)) {
        recipients.push_back(v);
      }
    }
    return recipients;
  }

  bool KnownBackedCandidate::note_fresh_statement(
      size_t statement_index_in_group,
      const network::vstaging::StatementKind &statement_kind) {
    bool really_fresh =
        !local_knowledge.contains(statement_index_in_group, statement_kind);
    local_knowledge.set(statement_index_in_group, statement_kind);
    return really_fresh;
  }

  void KnownBackedCandidate::sent_or_received_direct_statement(
      ValidatorIndex validator,
      size_t statement_index_in_group,
      const network::vstaging::StatementKind &statement_kind,
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
      ValidatorIndex validator,
      size_t statement_index_in_group,
      const network::vstaging::StatementKind &statement_kind) {
    auto it = mutual_knowledge.find(validator);
    if (it != mutual_knowledge.end() && it->second.local_knowledge
        && it->second.remote_knowledge) {
      return !it->second.remote_knowledge->contains(statement_index_in_group,
                                                    statement_kind);
    }
    return false;
  }

  std::optional<StatementFilter> KnownBackedCandidate::pending_statements(
      ValidatorIndex validator) {
    const StatementFilter &full_local = local_knowledge;
    auto it = mutual_knowledge.find(validator);
    if (it != mutual_knowledge.end() && it->second.local_knowledge
        && it->second.remote_knowledge) {
      const StatementFilter &remote = *it->second.remote_knowledge;
      StatementFilter result(full_local.seconded_in_group.bits.size());
      for (size_t i = 0; i < full_local.seconded_in_group.bits.size(); ++i) {
        result.seconded_in_group.bits[i] = full_local.seconded_in_group.bits[i]
                                        && !remote.seconded_in_group.bits[i];
        result.validated_in_group.bits[i] =
            full_local.validated_in_group.bits[i]
            && !remote.validated_in_group.bits[i];
      }
      return result;
    }
    return {};
  }

  std::optional<StatementFilter> ReceivedManifests::candidate_statement_filter(
      const CandidateHash &candidate_hash) {
    auto it = received.find(candidate_hash);
    if (it != received.end()) {
      return it->second
          .statement_knowledge;  // Assuming clone is similar to copy
    }
    return std::nullopt;
  }

  outcome::result<void> ReceivedManifests::import_received(
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
          || !std::ranges::includes(
              manifest_summary.statement_knowledge.seconded_in_group.bits,
              prev.statement_knowledge.seconded_in_group.bits)
          || !std::ranges::includes(
              manifest_summary.statement_knowledge.validated_in_group.bits,
              prev.statement_knowledge.validated_in_group.bits)) {
        return GridTracker::Error::CONFLICTING;
      }
      auto fresh_seconded =
          manifest_summary.statement_knowledge.seconded_in_group;
      for (size_t i = 0; i < fresh_seconded.bits.size(); ++i) {
        fresh_seconded.bits[i] =
            fresh_seconded.bits[i]
            || prev.statement_knowledge.seconded_in_group.bits[i];
      }

      bool within_limits = updating_ensure_within_seconding_limit(
          seconded_counts,
          manifest_summary.claimed_group_index,
          group_size,
          seconding_limit,
          fresh_seconded.bits);
      if (!within_limits) {
        return GridTracker::Error::SECONDING_OVERFLOW;
      }
      it->second = manifest_summary;
      return outcome::success();
    }
    // Entry is vacant
    bool within_limits = updating_ensure_within_seconding_limit(
        seconded_counts,
        manifest_summary.claimed_group_index,
        group_size,
        seconding_limit,
        manifest_summary.statement_knowledge.seconded_in_group.bits);
    if (within_limits) {
      received[candidate_hash] = manifest_summary;
      return outcome::success();
    }
    return GridTracker::Error::SECONDING_OVERFLOW;
  }

  bool ReceivedManifests::updating_ensure_within_seconding_limit(
      std::unordered_map<GroupIndex, std::vector<size_t>> &seconded_counts,
      GroupIndex group_index,
      size_t group_size,
      size_t seconding_limit,
      const std::vector<bool> &new_seconded) {
    if (seconding_limit == 0) {
      return false;
    }

    std::optional<std::reference_wrapper<std::vector<size_t>>> counts;
    auto it = seconded_counts.find(group_index);
    if (it == seconded_counts.end()) {
      auto i = seconded_counts.emplace(group_index,
                                       std::vector<size_t>(group_size, 0));
      counts = i.first->second;
    } else {
      counts = it->second;
    }

    for (size_t i = 0; i < new_seconded.size(); ++i) {
      if (new_seconded[i]) {
        if (counts->get()[i] == seconding_limit) {
          return false;
        }
      }
    }

    for (size_t i = 0; i < new_seconded.size(); ++i) {
      if (new_seconded[i]) {
        counts->get()[i] += 1;
      }
    }

    return true;
  }

}  // namespace kagome::parachain::grid
