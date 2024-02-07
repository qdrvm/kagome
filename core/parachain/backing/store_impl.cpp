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

  void BackingStoreImpl::remove(const BlockHash &relay_parent) {
//    backed_candidates_.erase(relay_parent);
//    if (auto it = candidates_.find(relay_parent); it != candidates_.end()) {
//      for (const auto &candidate : it->second) {
//        candidate_votes_.erase(candidate);
//      }
//      candidates_.erase(it);
//    }
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

  outcome::result<std::optional<BackingStore::ImportResult>> BackingStoreImpl::validity_vote(
    const std::unordered_map<ParachainId, std::vector<ValidatorIndex>>
          &groups,
    ValidatorIndex from,
    const CandidateHash &digest,
    const ValidityVote &vote) 
  {
    auto it = candidate_votes_.find(digest);
    if (it == candidate_votes_.end()) {
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

  outcome::result<std::optional<BackingStore::ImportResult>> BackingStoreImpl::import_candidate(
    const std::unordered_map<ParachainId, std::vector<ValidatorIndex>>
          &groups,
		ValidatorIndex authority,
		const network::CommittedCandidateReceipt &candidate,
		const ValidatorSignature &signature, bool allow_multiple_seconded
	) {
		const auto group = candidate.descriptor.para_id;
    if (auto it = groups.find(group); it == groups.end() || std::find(it->second.begin(), it->second.end(), authority) == it->second.end()) {
      return Error::UNAUTHORIZED_STATEMENT;      
    }

    const CandidateHash digest = candidateHash(*hasher_, candidate);
    bool new_proposal;
    if (auto it = authority_data_.find(authority); it != authority_data_.end()) {
      auto &existing = it->second;
      if (!allow_multiple_seconded && existing.proposals.size() == 1) {
        const auto &[old_digest, old_sig] = existing.proposals[0];
        if (old_digest != digest) {
						return Error::MULTIPLE_CANDIDATES;
					}
					new_proposal = false;
      } else if (allow_multiple_seconded && std::find_if(existing.proposals.begin(), existing.proposals.end(), [&digest](const auto &hash_and_sig) {
        const auto &[h, _] = hash_and_sig;
        return h == digest;
      }) != existing.proposals.end())					
				{
					new_proposal = false;
				} else {
					existing.proposals.emplace_back(digest, signature);
					new_proposal = true;
				}
    } else {
      auto &ad = authority_data_[authority];
      ad.proposals.emplace_back(digest, signature);
      new_proposal = true;
    }

		if (new_proposal) {
			auto &cv = candidate_votes_[digest];
      cv.candidate = candidate;
      cv.group_id = group;
		}

		return validity_vote(groups, authority, digest, ValidityVoteIssued{signature});
  }

  std::optional<BackingStore::ImportResult> BackingStoreImpl::put(
      const std::unordered_map<ParachainId, std::vector<ValidatorIndex>>
          &groups,
      Statement stm, bool allow_multiple_seconded) {
//    auto candidate_hash =
//        candidateHash(*hasher_, stm.payload.payload.candidate_state);

    const auto &signer = stm.payload.ix;
    const auto &signature = stm.signature;
    const auto &statement = stm.payload.payload;

    auto res = visit_in_place(statement.candidate_state,
      [&](const network::CommittedCandidateReceipt &candidate) {
        return import_candidate(groups, signer, candidate, signature, allow_multiple_seconded);
      },
      [&](const CandidateHash &digest) {
        return validity_vote(groups, signer, digest, ValidityVoteValid{signature});
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
  BackingStoreImpl::get_validity_votes(
      const network::CandidateHash &candidate_hash) const {
    if (auto it = candidate_votes_.find(candidate_hash); it != candidate_votes_.end()) {
      return {{it->second}};
    }
    return std::nullopt;
  }

  void BackingStoreImpl::add(const BlockHash &relay_parent,
                             BackedCandidate &&candidate) {
    backed_candidates_[relay_parent].emplace_back(std::move(candidate));
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
