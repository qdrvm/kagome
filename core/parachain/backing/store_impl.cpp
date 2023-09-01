/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/backing/store_impl.hpp"

namespace kagome::parachain {
  using network::BackedCandidate;
  using network::CommittedCandidateReceipt;
  using network::ValidityAttestation;

  BackingStoreImpl::BackingStoreImpl(std::shared_ptr<crypto::Hasher> hasher)
      : hasher_{std::move(hasher)} {}

  void BackingStoreImpl::remove(const BlockHash &relay_parent) {
    backed_candidates_.erase(relay_parent);
    if (auto it = candidates_.find(relay_parent); it != candidates_.end()) {
      for (const auto &candidate : it->second) {
        statements_.erase(candidate);
      }
      candidates_.erase(it);
    }
  }

  bool BackingStoreImpl::is_in_group(
      const std::unordered_map<ParachainId, std::vector<ValidatorIndex>>
          &groups,
      GroupIndex group,
      ValidatorIndex authority) {
    if (auto it = groups.find(group); it != groups.end()) {
      for (const auto a : it->second) {
        if (a == authority) {
          return true;
        }
      }
    }
    return false;
  }

  std::optional<BackingStore::ImportResult> BackingStoreImpl::put(
      const std::unordered_map<ParachainId, std::vector<ValidatorIndex>>
          &groups,
      Statement statement) {
    auto candidate_hash =
        candidateHash(*hasher_, statement.payload.payload.candidate_state);
    StatementInfo *s{nullptr};
    if (auto s_it = statements_.find(candidate_hash);
        s_it != statements_.end()) {
      if (!is_in_group(groups, s_it->second.first, statement.payload.ix)) {
        return std::nullopt;
      }
      s = &s_it->second;
      s->second.emplace(statement.payload.ix,
                        ValidityVoteValid{std::move(statement)});
    } else if (auto seconded{boost::get<network::CommittedCandidateReceipt>(
                   &statement.payload.payload.candidate_state)}) {
      const auto group = seconded->descriptor.para_id;
      if (!is_in_group(groups, group, statement.payload.ix)) {
        return std::nullopt;
      }

      s = &statements_[candidate_hash];
      s->first = seconded->descriptor.para_id;
      candidates_[seconded->descriptor.relay_parent].emplace(candidate_hash);
      s->second.emplace(statement.payload.ix,
                        ValidityVoteIssued{std::move(statement)});
    } else {
      return std::nullopt;
    }

    return BackingStore::ImportResult{
        .candidate = candidate_hash,
        .group_id = s->first,
        .validity_votes = s->second.size(),
    };
  }

  std::optional<std::reference_wrapper<const BackingStore::StatementInfo>>
  BackingStoreImpl::get_validity_votes(
      const network::CandidateHash &candidate_hash) const {
    if (auto it = statements_.find(candidate_hash); it != statements_.end()) {
      return {{it->second}};
    }
    return std::nullopt;
  }

  void BackingStoreImpl::add(const BlockHash &relay_parent,
                             BackedCandidate &&candidate) {
    backed_candidates_[relay_parent].emplace_back(std::move(candidate));
  }

  std::optional<network::CommittedCandidateReceipt>
  BackingStoreImpl::get_candidate(
      const network::CandidateHash &candidate_hash) const {
    if (auto it = statements_.find(candidate_hash); it != statements_.end()) {
      for (auto &[_, validity_vote] : it->second.second) {
        const auto &statement = visit_in_place(
            validity_vote,
            [](const auto &val) -> std::reference_wrapper<Statement> {
              return {(Statement &)val};
            });

        if (auto seconded =
                boost::get<const network::CommittedCandidateReceipt>(
                    &statement.get().payload.payload.candidate_state)) {
          return *seconded;
        }
      }
    }
    return std::nullopt;
  }

  std::vector<BackedCandidate> BackingStoreImpl::get(
      const BlockHash &relay_parent) const {
    if (auto it = backed_candidates_.find(relay_parent);
        it != backed_candidates_.end()) {
      return it->second;
    }
    return {};
  }
}  // namespace kagome::parachain
