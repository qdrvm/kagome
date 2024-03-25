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

/**
 * @file store_impl.hpp
 * @brief This file contains the BackingStoreImpl class, which is an
 * implementation of the BackingStore interface.
 *
 * The BackingStoreImpl class is used to manage and store statements and
 * backed candidates for active backing tasks. It provides methods to add,
 * remove, and retrieve backed candidates and their associated statements.
 *
 * The class contains the following public methods:
 * - put: This method is used to add a statement to the store. It checks if
 * the statement is valid and if it is, adds it to the store.
 * - get: This method is used to retrieve backed candidates associated with a
 * given relay parent.
 * - add: This method is used to add a backed candidate to the store.
 * - get_candidate: This method is used to retrieve a candidate receipt
 * associated with a given candidate hash.
 * - get_validity_votes: This method is used to retrieve validity votes
 * associated with a given candidate hash.
 * - remove: This method is used to remove all data associated with a given
 * relay parent from the store.
 *
 * The class also contains private methods and data members used for internal
 * operations.
 *
 * @author Quadrivium LLC
 * @copyright Quadrivium LLC
 */

namespace kagome::parachain {
  /**
   * @class BackingStoreImpl
   * @brief This class is an implementation of the BackingStore interface.
   */
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

    /**
     * @brief This method is used to add a statement to the store. It checks if
     * the statement is valid and if it is, adds it to the store.
     *  @param groups A map of parachain IDs to validator indices.
     *  @param statement The statement to add to the store.
     *  @return An optional ImportResult object containing the result of the
     * import operation
     */
    std::optional<ImportResult> put(
        const RelayHash &relay_parent,
        const std::unordered_map<ParachainId, std::vector<ValidatorIndex>>
            &groups,
        Statement statement,
        bool allow_multiple_seconded) override;

    /**
     * @method get
     * @brief This method is used to retrieve backed candidates associated with
     * a given relay parent.
     * @param relay_parent The relay parent associated with the backed
     * candidates.
     * @return A vector of BackedCandidate objects.
     */
    std::vector<BackedCandidate> get(
        const BlockHash &relay_parent) const override;

    /**
     * @method add
     * @brief This method is used to add a backed candidate to the store.
     * @param relay_parent The relay parent associated with the backed
     * candidate.
     * @param candidate The backed candidate to be added to the store.
     */
    void add(const BlockHash &relay_parent,
             BackedCandidate &&candidate) override;

    std::optional<std::reference_wrapper<const StatementInfo>> getCadidateInfo(
        const RelayHash &relay_parent,
        const network::CandidateHash &candidate_hash) const override;

    void onActivateLeaf(const RelayHash &relay_parent) override;
    void onDeactivateLeaf(const RelayHash &relay_parent) override;

   private:
    struct AuthorityData {
      std::deque<std::pair<CandidateHash, ValidatorSignature>> proposals;
    };

    struct PerRelayParent {
      std::vector<BackedCandidate> backed_candidates_;
      std::unordered_map<ValidatorIndex, AuthorityData> authority_data_;
      std::unordered_map<CandidateHash, StatementInfo> candidate_votes_;
    };

    template <typename F>
    void forRelayState(const RelayHash &relay_parent, F &&f) {
      if (auto it = per_relay_parent_.find(relay_parent);
          it != per_relay_parent_.end()) {
        std::forward<F>(f)(it->second);
      }
    }

    template <typename F>
    void forRelayState(const RelayHash &relay_parent, F &&f) const {
      if (auto it = per_relay_parent_.find(relay_parent);
          it != per_relay_parent_.end()) {
        std::forward<F>(f)(it->second);
      }
    }

    bool is_in_group(
        const std::unordered_map<ParachainId, std::vector<ValidatorIndex>>
            &groups,
        GroupIndex group,
        ValidatorIndex authority);

    outcome::result<std::optional<BackingStore::ImportResult>> validity_vote(
        PerRelayParent &state,
        const std::unordered_map<ParachainId, std::vector<ValidatorIndex>>
            &groups,
        ValidatorIndex from,
        const CandidateHash &digest,
        const ValidityVote &vote);
    outcome::result<std::optional<BackingStore::ImportResult>> import_candidate(
        PerRelayParent &state,
        const std::unordered_map<ParachainId, std::vector<ValidatorIndex>>
            &groups,
        ValidatorIndex authority,
        const network::CommittedCandidateReceipt &candidate,
        const ValidatorSignature &signature,
        bool allow_multiple_seconded);

    std::shared_ptr<crypto::Hasher> hasher_;
    std::unordered_map<RelayHash, PerRelayParent> per_relay_parent_;
  };
}  // namespace kagome::parachain

OUTCOME_HPP_DECLARE_ERROR(kagome::parachain, BackingStoreImpl::Error)
