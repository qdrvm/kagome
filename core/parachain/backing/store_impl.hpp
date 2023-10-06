/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_BACKING_STORE_IMPL_HPP
#define KAGOME_PARACHAIN_BACKING_STORE_IMPL_HPP

#include "parachain/backing/store.hpp"

#include <map>
#include <unordered_map>
#include <unordered_set>

namespace kagome::parachain {
  class BackingStoreImpl : public BackingStore {
    using ValidatorIndex = network::ValidatorIndex;

   public:
    BackingStoreImpl(std::shared_ptr<crypto::Hasher> hasher);

    std::optional<ImportResult> put(
        const std::unordered_map<ParachainId, std::vector<ValidatorIndex>>
            &groups,
        Statement statement) override;
    std::vector<BackedCandidate> get(
        const BlockHash &relay_parent) const override;
    void add(const BlockHash &relay_parent,
             BackedCandidate &&candidate) override;

    std::optional<network::CommittedCandidateReceipt> get_candidate(
        const network::CandidateHash &candidate_hash) const override;

    std::optional<std::reference_wrapper<const StatementInfo>>
    get_validity_votes(
        const network::CandidateHash &candidate_hash) const override;

    void remove(const BlockHash &relay_parent) override;

   private:
    bool is_in_group(
        const std::unordered_map<ParachainId, std::vector<ValidatorIndex>>
            &groups,
        GroupIndex group,
        ValidatorIndex authority);
    std::shared_ptr<crypto::Hasher> hasher_;

    std::unordered_map<BlockHash, std::unordered_set<BlockHash>> candidates_;
    std::unordered_map<BlockHash, StatementInfo> statements_;
    std::unordered_map<BlockHash, std::vector<BackedCandidate>>
        backed_candidates_;
  };
}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_BACKING_STORE_IMPL_HPP
