/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/backing/store_impl.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::parachain, BackingStoreImpl::Error, e) {
  using E = decltype(e);
  switch (e) {
    case E::UNAUTHORIZED_STATEMENT:
      return "Unauthorized statement";
    case E::DOUBLE_VOTE:
      return "Double vote";
    case E::MULTIPLE_CANDIDATES:
      return "Multiple candidates";
    case E::CRITICAL_ERROR:
      return "Critical error";
  }
  return "unknown error (invalid BackingStoreImpl::Error";
}

namespace kagome::parachain {
  using network::BackedCandidate;
  using network::CommittedCandidateReceipt;
  using network::ValidityAttestation;

  BackingStoreImpl::BackingStoreImpl(std::shared_ptr<crypto::Hasher> hasher)
      : hasher_{std::move(hasher)} {}

  void BackingStoreImpl::onDeactivateLeaf(const BlockHash &relay_parent) {
    per_relay_parent_.erase(relay_parent);
  }

  void BackingStoreImpl::onActivateLeaf(const BlockHash &relay_parent) {
    std::ignore = per_relay_parent_[relay_parent];
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

  outcome::result<std::optional<BackingStore::ImportResult>>
  BackingStoreImpl::validity_vote(
      PerRelayParent &state,
      const std::unordered_map<ParachainId, std::vector<ValidatorIndex>>
          &groups,
      ValidatorIndex from,
      const CandidateHash &digest,
      const ValidityVote &vote) {
    auto it = state.candidate_votes_.find(digest);
    if (it == state.candidate_votes_.end()) {
      return std::nullopt;
    }
    BackingStore::StatementInfo &votes = it->second;

    if (!is_in_group(groups, votes.group_id, from)) {
      return Error::UNAUTHORIZED_STATEMENT;
    }

    auto i = votes.validity_votes.find(from);
    if (i != votes.validity_votes.end()) {
      if (i->second != vote) {
        return Error::DOUBLE_VOTE;
      }
      return std::nullopt;
    }

    votes.validity_votes[from] = vote;
    return BackingStore::ImportResult{
        .candidate = digest,
        .group_id = votes.group_id,
        .validity_votes = votes.validity_votes.size(),
    };
  }

  outcome::result<std::optional<BackingStore::ImportResult>>
  BackingStoreImpl::import_candidate(
      PerRelayParent &state,
      GroupIndex group_id,
      const std::unordered_map<CoreIndex, std::vector<ValidatorIndex>> &groups,
      ValidatorIndex authority,
      const network::CommittedCandidateReceipt &candidate,
      const ValidatorSignature &signature,
      bool allow_multiple_seconded) {
    if (auto it = groups.find(group_id);
        it == groups.end()
        || std::find(it->second.begin(), it->second.end(), authority)
               == it->second.end()) {
      return Error::UNAUTHORIZED_STATEMENT;
    }

    const CandidateHash digest = candidateHash(*hasher_, candidate);
    bool new_proposal;
    if (auto it = state.authority_data_.find(authority);
        it != state.authority_data_.end()) {
      auto &existing = it->second;
      if (!allow_multiple_seconded && existing.proposals.size() == 1) {
        const auto &[old_digest, old_sig] = existing.proposals[0];
        if (old_digest != digest) {
          return Error::MULTIPLE_CANDIDATES;
        }
        new_proposal = false;
      } else if (allow_multiple_seconded
                 && std::find_if(existing.proposals.begin(),
                                 existing.proposals.end(),
                                 [&digest](const auto &hash_and_sig) {
                                   const auto &[h, _] = hash_and_sig;
                                   return h == digest;
                                 })
                        != existing.proposals.end()) {
        new_proposal = false;
      } else {
        existing.proposals.emplace_back(digest, signature);
        new_proposal = true;
      }
    } else {
      auto &ad = state.authority_data_[authority];
      ad.proposals.emplace_back(digest, signature);
      new_proposal = true;
    }

    if (new_proposal) {
      auto &cv = state.candidate_votes_[digest];
      cv.candidate = candidate;
      cv.group_id = group_id;
    }

    return validity_vote(
        state, groups, authority, digest, ValidityVoteIssued{signature});
  }

  std::optional<BackingStore::ImportResult> BackingStoreImpl::put(
      const RelayHash &relay_parent,
      GroupIndex group_id,
      const std::unordered_map<CoreIndex, std::vector<ValidatorIndex>> &groups,
      Statement stm,
      bool allow_multiple_seconded) {
    std::optional<std::reference_wrapper<PerRelayParent>> per_rp_state;
    forRelayState(relay_parent,
                  [&](PerRelayParent &state) { per_rp_state = state; });

    if (!per_rp_state) {
      return std::nullopt;
    }

    const auto &signer = stm.payload.ix;
    const auto &signature = stm.signature;
    const auto &statement = stm.payload.payload;

    auto res = visit_in_place(
        statement.candidate_state,
        [&](const network::CommittedCandidateReceipt &candidate) {
          return import_candidate(per_rp_state->get(),
                                  group_id,
                                  groups,
                                  signer,
                                  candidate,
                                  signature,
                                  allow_multiple_seconded);
        },
        [&](const CandidateHash &digest) {
          return validity_vote(per_rp_state->get(),
                               groups,
                               signer,
                               digest,
                               ValidityVoteValid{signature});
        },
        [](const auto &) {
          UNREACHABLE;
          return Error::CRITICAL_ERROR;
        });

    if (res.has_error()) {
      return std::nullopt;
    }
    return res.value();
  }

  std::optional<std::reference_wrapper<const BackingStore::StatementInfo>>
  BackingStoreImpl::getCadidateInfo(
      const RelayHash &relay_parent,
      const network::CandidateHash &candidate_hash) const {
    std::optional<std::reference_wrapper<const BackingStore::StatementInfo>>
        out;
    forRelayState(relay_parent, [&](const PerRelayParent &state) {
      if (auto it = state.candidate_votes_.find(candidate_hash);
          it != state.candidate_votes_.end()) {
        out = it->second;
      }
    });
    return out;
  }

  void BackingStoreImpl::add(const BlockHash &relay_parent,
                             BackedCandidate &&candidate) {
    forRelayState(relay_parent, [&](PerRelayParent &state) {
      state.backed_candidates_.emplace_back(std::move(candidate));
    });
  }

  std::vector<BackedCandidate> BackingStoreImpl::get(
      const BlockHash &relay_parent) const {
    std::vector<BackedCandidate> out;
    forRelayState(relay_parent, [&](const PerRelayParent &state) {
      out = state.backed_candidates_;
    });
    return out;
  }
}  // namespace kagome::parachain
