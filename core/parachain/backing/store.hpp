/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/tagged.hpp"
#include "log/logger.hpp"
#include "network/types/collator_messages.hpp"

namespace kagome::parachain {

  /// Used to keep statements and backed candidates for active backing task.
  /// Chains Block producer with backing, which main purpose to propose valid
  /// candidates from parachains.
  class BackingStore {
   public:
    using BlockHash = primitives::BlockHash;
    using Statement = network::SignedStatement;
    using BackedCandidate = network::BackedCandidate;

    struct ImportResult {
      /// The digest of the candidate.
      primitives::BlockHash candidate;
      /// The group that the candidate is in.
      network::ParachainId group_id;
      /// How many validity votes are currently witnessed.
      uint64_t validity_votes;
    };

    using ValidityVoteIssued = Tagged<ValidatorSignature, struct Issued>;
    using ValidityVoteValid = Tagged<ValidatorSignature, struct Valid>;
    using ValidityVote = boost::variant<ValidityVoteIssued, ValidityVoteValid>;

    struct StatementInfo {
      network::ParachainId group_id;
      network::CommittedCandidateReceipt candidate;
      std::map<ValidatorIndex, ValidityVote> validity_votes;
    };

    virtual ~BackingStore() = default;

    virtual std::optional<ImportResult> put(
        const RelayHash &relay_parent,
        GroupIndex group_id,
        const std::unordered_map<CoreIndex, std::vector<ValidatorIndex>>
            &groups,
        Statement statement,
        bool allow_multiple_seconded) = 0;

    virtual std::vector<BackedCandidate> get(
        const BlockHash &relay_parent) const = 0;

    virtual void onActivateLeaf(const RelayHash &relay_parent) = 0;
    virtual void onDeactivateLeaf(const RelayHash &relay_parent) = 0;

    virtual void add(const BlockHash &relay_parent,
                     BackedCandidate &&candidate) = 0;

    virtual std::optional<std::reference_wrapper<const StatementInfo>>
    getCadidateInfo(const RelayHash &relay_parent,
                    const network::CandidateHash &candidate_hash) const = 0;
    virtual void printStoragesLoad() = 0;
  };
}  // namespace kagome::parachain
