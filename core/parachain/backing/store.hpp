/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_BACKING_STORE_HPP
#define KAGOME_PARACHAIN_BACKING_STORE_HPP

#include "network/types/collator_messages.hpp"
#include "common/tagged.hpp"

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

    using ValidityVoteIssued = Tagged<Statement, struct Issued>;
    using ValidityVoteValid = Tagged<Statement, struct Valid>;  
    using ValidityVote = boost::variant<
       ValidityVoteIssued,
       ValidityVoteValid
    >;

    using StatementInfo =
        std::pair<network::ParachainId, std::map<ValidatorIndex, ValidityVote>>;

    virtual ~BackingStore() = default;

    virtual std::optional<ImportResult> put(
        std::unordered_map<ParachainId, std::vector<ValidatorIndex>> const
            &groups,
        Statement statement) = 0;

    virtual std::vector<BackedCandidate> get(
        const BlockHash &relay_parent) const = 0;

    virtual void remove(const BlockHash &relay_parent) = 0;

    virtual void add(const BlockHash &relay_parent,
                     BackedCandidate &&candidate) = 0;

    virtual std::optional<network::CommittedCandidateReceipt> get_candidate(
        network::CandidateHash const &candidate_hash) const = 0;

    virtual std::optional<std::reference_wrapper<StatementInfo const>>
    get_validity_votes(network::CandidateHash const &candidate_hash) const = 0;
  };
}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_BACKING_STORE_HPP
