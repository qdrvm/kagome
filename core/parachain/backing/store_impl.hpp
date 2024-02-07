/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "parachain/backing/store.hpp"

#include <map>
#include <unordered_map>
#include <unordered_set>

namespace kagome::parachain {
  class BackingStoreImpl : public BackingStore {
    using ValidatorIndex = network::ValidatorIndex;

   public:
    enum Error {
      UNAUTHORIZED_STATEMENT = 1,
      DOUBLE_VOTE,
      MULTIPLE_CANDIDATES,
      CRITICAL_ERROR,
    };

    BackingStoreImpl(std::shared_ptr<crypto::Hasher> hasher);

    std::optional<ImportResult> put(
        const std::unordered_map<ParachainId, std::vector<ValidatorIndex>>
            &groups,
        Statement statement, bool allow_multiple_seconded) override;
    std::vector<BackedCandidate> get(
        const BlockHash &relay_parent) const override;
    void add(const BlockHash &relay_parent,
             BackedCandidate &&candidate) override;

    std::optional<std::reference_wrapper<const StatementInfo>>
    get_validity_votes(
        const network::CandidateHash &candidate_hash) const override;

    void remove(const BlockHash &relay_parent) override;

   private:
    struct AuthorityData {
	    std::deque<std::pair<CandidateHash, ValidatorSignature>> proposals;
    };

    bool is_in_group(
        const std::unordered_map<ParachainId, std::vector<ValidatorIndex>>
            &groups,
        GroupIndex group,
        ValidatorIndex authority);

  outcome::result<std::optional<BackingStore::ImportResult>> validity_vote(const std::unordered_map<ParachainId, std::vector<ValidatorIndex>>
          &groups, ValidatorIndex from,const CandidateHash &digest, const ValidityVote &vote);
          outcome::result<std::optional<BackingStore::ImportResult>> import_candidate(
            const std::unordered_map<ParachainId, std::vector<ValidatorIndex>>
          &groups,
		ValidatorIndex authority,
		const network::CommittedCandidateReceipt &candidate,
		const ValidatorSignature &signature, bool allow_multiple_seconded
	);

    std::shared_ptr<crypto::Hasher> hasher_;

//    std::unordered_map<BlockHash, std::unordered_set<BlockHash>> candidates_;
    std::unordered_map<BlockHash, std::vector<BackedCandidate>>
        backed_candidates_;


    std::unordered_map<ValidatorIndex, AuthorityData> authority_data_;
    std::unordered_map<CandidateHash, StatementInfo> candidate_votes_;
  };
}  // namespace kagome::parachain

OUTCOME_HPP_DECLARE_ERROR(kagome::parachain, BackingStoreImpl::Error)
