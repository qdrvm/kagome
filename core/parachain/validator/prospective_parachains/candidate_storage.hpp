/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/variant.hpp>
#include <map>
#include <optional>
#include <ranges>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "outcome/outcome.hpp"

#include "crypto/hasher/hasher_impl.hpp"
#include "log/logger.hpp"
#include "network/types/collator_messages.hpp"
#include "network/types/collator_messages_vstaging.hpp"
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
    /// The collator that created the candidate.
    CollatorId collator;
    /// The signature of the collator on the payload.
    runtime::CollatorSignature collator_signature;
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

  struct CandidateEntry {
    CandidateHash candidate_hash;
    RelayHash relay_parent;
    ProspectiveCandidate candidate;
    CandidateState state;
  };

  struct CandidateStorage {
    enum class Error {
      CANDIDATE_ALREADY_KNOWN,
      PERSISTED_VALIDATION_DATA_MISMATCH,
    };

    outcome::result<void> addCandidate(
        const CandidateHash &candidate_hash,
        const network::CommittedCandidateReceipt &candidate,
        const crypto::Hashed<const runtime::PersistedValidationData &,
                             32,
                             crypto::Blake2b_StreamHasher<32>>
            &persisted_validation_data,
        const std::shared_ptr<crypto::Hasher> &hasher);

    Option<std::reference_wrapper<const CandidateEntry>> get(
        const CandidateHash &candidate_hash) const;

    Option<Hash> relayParentByCandidateHash(
        const CandidateHash &candidate_hash) const;

    bool contains(const CandidateHash &candidate_hash) const;

    template <typename F>
    void iterParaChildren(const Hash &parent_head_hash, F &&func) const;

    Option<std::reference_wrapper<const HeadData>> headDataByHash(
        const Hash &hash) const;

    void removeCandidate(const CandidateHash &candidate_hash,
                         const std::shared_ptr<crypto::Hasher> &hasher);

    template <typename F>
    void retain(F &&pred /*bool(CandidateHash)*/);

    void markSeconded(const CandidateHash &candidate_hash);

    void markBacked(const CandidateHash &candidate_hash);

    bool isBacked(const CandidateHash &candidate_hash) const;

    std::pair<size_t, size_t> len() const;

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
