/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "parachain/validator/prospective_parachains/candidate_storage.hpp"
#include "parachain/validator/prospective_parachains/common.hpp"

namespace kagome::parachain::fragment {

  struct RelayChainBlockInfo {
    /// The hash of the relay-chain block.
    Hash hash;
    /// The number of the relay-chain block.
    BlockNumber number;
    /// The storage-root of the relay-chain block.
    Hash storage_root;
  };

  struct Fragment {
    enum Error {
      HRMP_MESSAGE_DESCENDING_OR_DUPLICATE = 1,
      PERSISTED_VALIDATION_DATA_MISMATCH,
      VALIDATION_CODE_MISMATCH,
      RELAY_PARENT_TOO_OLD,
      CODE_UPGRADE_RESTRICTED,
      CODE_SIZE_TOO_LARGE,
      DMP_ADVANCEMENT_RULE,
      HRMP_MESSAGES_PER_CANDIDATE_OVERFLOW,
      UMP_MESSAGES_PER_CANDIDATE_OVERFLOW,
    };

    /// The new relay-parent.
    RelayChainBlockInfo relay_parent;
    /// The constraints this fragment is operating under.
    Constraints operating_constraints;
    /// The core information about the prospective candidate.
    std::shared_ptr<const ProspectiveCandidate> candidate;
    /// Modifications to the constraints based on the outputs of
    /// the candidate.
    ConstraintModifications modifications;

    /// Access the relay parent information.
    const RelayChainBlockInfo &relay_parent() const;

    /// Modifications to constraints based on the outputs of the candidate.
    const ConstraintModifications &constraint_modifications() const;

    /// Access the operating constraints
    const Constraints &operating_constraints() const;

    /// Access the underlying prospective candidate.
    const ProspectiveCandidate &candidate() const;

    /// Get a cheap ref-counted copy of the underlying prospective candidate.
    std::shared_ptr<const ProspectiveCandidate> candidate_clone() const;

    static outcome::result<Fragment> create(
        const RelayChainBlockInfo &relay_parent,
        const Constraints &operating_constraints,
        const std::shared_ptr<const ProspectiveCandidate> &candidate);

    /// Check the candidate against the operating constrains and return the
    /// constraint modifications
    /// made by this candidate.
    static outcome::result<ConstraintModifications> check_against_constraints(
        const RelayChainBlockInfo &relay_parent,
        const Constraints &operating_constraints,
        const CandidateCommitments &commitments,
        const ValidationCodeHash &validation_code_hash,
        const PersistedValidationData &persisted_validation_data);
  };
}  // namespace kagome::parachain::fragment

OUTCOME_HPP_DECLARE_ERROR(kagome::parachain::fragment, Fragment::Error)
