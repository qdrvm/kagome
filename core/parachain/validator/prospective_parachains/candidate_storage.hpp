/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "parachain/validator/prospective_parachains/common.hpp"

#include "crypto/hasher/hasher_impl.hpp"
#include "log/logger.hpp"
#include "parachain/types.hpp"
#include "parachain/validator/collations.hpp"
#include "primitives/common.hpp"
#include "primitives/math.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"
#include "utils/map.hpp"

namespace kagome::parachain::fragment {

  struct ProspectiveCandidate {
    /// The commitments to the output of the execution.
    network::CandidateCommitments commitments;
    /// The persisted validation data used to create the candidate.
    runtime::PersistedValidationData persisted_validation_data;
    /// The hash of the PoV.
    Hash pov_hash;
    /// The validation code hash used by the candidate.
    ValidationCodeHash validation_code_hash;
  };

  /// The state of a candidate.
  ///
  /// Candidates aren't even considered until they've at least been seconded.
  enum CandidateState {
    /// The candidate has been introduced in a spam-protected way but
    /// is not necessarily backed.
    Introduced,
    /// The candidate has been seconded.
    Seconded,
    /// The candidate has been completely backed by the group.
    Backed,
  };

  /// Representation of a candidate into the [`CandidateStorage`].
  struct CandidateEntry {
    CandidateHash candidate_hash;
    Hash parent_head_data_hash;
    Hash output_head_data_hash;
    RelayHash relay_parent;
    ProspectiveCandidate candidate;
    CandidateState state;

    static outcome::result<CandidateEntry> create_seconded(
        const CandidateHash &candidate_hash,
        const network::CommittedCandidateReceipt &candidate,
        const crypto::Hashed<const runtime::PersistedValidationData &,
                             32,
                             crypto::Blake2b_StreamHasher<32>>
            &persisted_validation_data,
        const std::shared_ptr<crypto::Hasher> &hasher);

    static outcome::result<CandidateEntry> create(
        const CandidateHash &candidate_hash,
        const network::CommittedCandidateReceipt &candidate,
        const crypto::Hashed<const runtime::PersistedValidationData &,
                             32,
                             crypto::Blake2b_StreamHasher<32>>
            &persisted_validation_data,
        CandidateState state,
        const std::shared_ptr<crypto::Hasher> &hasher);
  };

  struct CandidateStorage {
    enum class Error {
      CANDIDATE_ALREADY_KNOWN,
      PERSISTED_VALIDATION_DATA_MISMATCH,
      ZERO_LENGTH_CYCLE,
    };

    /// Introduce a new candidate entry.
    outcome::result<void> add_candidate_entry(CandidateEntry candidate);
    outcome::result<void> add_pending_availability_candidate(
        const CandidateHash &candidate_hash,
        const network::CommittedCandidateReceipt &candidate,
        const crypto::Hashed<const runtime::PersistedValidationData &,
                             32,
                             crypto::Blake2b_StreamHasher<32>>
            &persisted_validation_data,
        const std::shared_ptr<crypto::Hasher> &hasher);

    bool contains(const CandidateHash &candidate_hash) const;

    /// Returns the backed candidates which have the given head data hash as
    /// parent.
    template <typename F>
    void possible_backed_para_children(const Hash &parent_head_hash,
                                       F &&func) const {
      if (auto it = by_parent_head.find(parent_head_hash);
          it != by_parent_head.end()) {
        for (const auto &h : it->second) {
          if (auto c_it = by_candidate_hash.find(h);
              c_it != by_candidate_hash.end()
              && c_it->second.state == CandidateState::Backed) {
            std::forward<F>(func)(c_it->second);
          }
        }
      }
    }

    Option<std::reference_wrapper<const CandidateEntry>> get(
        const CandidateHash &candidate_hash) const;

    Option<std::reference_wrapper<const HeadData>> head_data_by_hash(
        const Hash &hash) const;

    void remove_candidate(const CandidateHash &candidate_hash,
                          const std::shared_ptr<crypto::Hasher> &hasher);

    template <typename F>
    void retain(F &&pred /*bool(CandidateHash)*/);

    void markSeconded(const CandidateHash &candidate_hash);

    void mark_backed(const CandidateHash &candidate_hash);

    bool isBacked(const CandidateHash &candidate_hash) const;

    size_t len() const;

   private:
    // Index from head data hash to candidate hashes with that head data as a
    // parent.
    HashMap<Hash, HashSet<CandidateHash>> by_parent_head;

    // Index from head data hash to candidate hashes outputting that head data.
    HashMap<Hash, HashSet<CandidateHash>> by_output_head;

    // Index from candidate hash to fragment node.
    HashMap<CandidateHash, CandidateEntry> by_candidate_hash;
  };

}  // namespace kagome::parachain::fragment

OUTCOME_HPP_DECLARE_ERROR(kagome::parachain::fragment, CandidateStorage::Error);
