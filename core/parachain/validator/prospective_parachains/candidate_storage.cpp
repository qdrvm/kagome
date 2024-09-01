/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/prospective_parachains/candidate_storage.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::parachain::fragment,
                            CandidateStorage::Error,
                            e) {
  using E = kagome::parachain::fragment::CandidateStorage::Error;
  switch (e) {
    case E::CANDIDATE_ALREADY_KNOWN:
      return "Candidate already known";
    case E::PERSISTED_VALIDATION_DATA_MISMATCH:
      return "Persisted validation data mismatch";
    case E::ZERO_LENGTH_CYCLE:
      return "Zero length zycle";
  }
  return "Unknown error";
}

namespace kagome::parachain::fragment {

  outcome::result<CandidateEntry> CandidateEntry::create_seconded(
      const CandidateHash &candidate_hash,
      const network::CommittedCandidateReceipt &candidate,
      const crypto::Hashed<const runtime::PersistedValidationData &,
                           32,
                           crypto::Blake2b_StreamHasher<32>>
          &persisted_validation_data,
      const std::shared_ptr<crypto::Hasher> &hasher) {
    return CandidateEntry::create(candidate_hash,
                                  candidate,
                                  persisted_validation_data,
                                  CandidateState::Seconded,
                                  hasher);
  }

  outcome::result<CandidateEntry> CandidateEntry::create(
      const CandidateHash &candidate_hash,
      const network::CommittedCandidateReceipt &candidate,
      const crypto::Hashed<const runtime::PersistedValidationData &,
                           32,
                           crypto::Blake2b_StreamHasher<32>>
          &persisted_validation_data,
      CandidateState state,
      const std::shared_ptr<crypto::Hasher> &hasher) {
    if (persisted_validation_data.getHash()
        != candidate.descriptor.persisted_data_hash) {
      return CandidateStorage::Error::PERSISTED_VALIDATION_DATA_MISMATCH;
    }

    const auto parent_head_data_hash =
        hasher->blake2b_256(persisted_validation_data.get().parent_head);
    const auto output_head_data_hash =
        hasher->blake2b_256(candidate.commitments.para_head);

    if (parent_head_data_hash == output_head_data_hash) {
      return CandidateStorage::Error::ZERO_LENGTH_CYCLE;
    }

    return CandidateEntry{
        .candidate_hash = candidate_hash,
        .parent_head_data_hash = parent_head_data_hash,
        .output_head_data_hash = output_head_data_hash,
        .relay_parent = candidate.descriptor.relay_parent,
        .candidate = std::make_shared<ProspectiveCandidate>(
            candidate.commitments,
            persisted_validation_data.get(),
            candidate.descriptor.pov_hash,
            candidate.descriptor.validation_code_hash),
        .state = state,
    };
  }

  Option<Ref<const CandidateCommitments>> CandidateEntry::get_commitments()
      const {
    BOOST_ASSERT_MSG(candidate, "Candidate is undefined!");
    return std::cref(candidate->commitments);
  }

  Option<Ref<const PersistedValidationData>>
  CandidateEntry::get_persisted_validation_data() const {
    BOOST_ASSERT_MSG(candidate, "Candidate is undefined!");
    return std::cref(candidate->persisted_validation_data);
  }

  Option<Ref<const ValidationCodeHash>>
  CandidateEntry::get_validation_code_hash() const {
    BOOST_ASSERT_MSG(candidate, "Candidate is undefined!");
    return std::cref(candidate->validation_code_hash);
  }

  Hash CandidateEntry::get_parent_head_data_hash() const {
    return parent_head_data_hash;
  }

  Option<Hash> CandidateEntry::get_output_head_data_hash() const {
    return output_head_data_hash;
  }

  Hash CandidateEntry::get_relay_parent() const {
    return relay_parent;
  }

  CandidateHash CandidateEntry::get_candidate_hash() const {
    return candidate_hash;
  }

  outcome::result<void> CandidateStorage::add_candidate_entry(
      CandidateEntry candidate) {
    const auto candidate_hash = candidate.candidate_hash;
    if (by_candidate_hash.contains(candidate_hash)) {
      return Error::CANDIDATE_ALREADY_KNOWN;
    }

    by_parent_head[candidate.parent_head_data_hash].emplace(candidate_hash);
    by_output_head[candidate.output_head_data_hash].emplace(candidate_hash);
    by_candidate_hash.emplace(candidate_hash, std::move(candidate));

    return outcome::success();
  }

  outcome::result<void> CandidateStorage::add_pending_availability_candidate(
      const CandidateHash &candidate_hash,
      const network::CommittedCandidateReceipt &candidate,
      const crypto::Hashed<const runtime::PersistedValidationData &,
                           32,
                           crypto::Blake2b_StreamHasher<32>>
          &persisted_validation_data,
      const std::shared_ptr<crypto::Hasher> &hasher) {
    OUTCOME_TRY(entry,
                CandidateEntry::create(candidate_hash,
                                       candidate,
                                       persisted_validation_data,
                                       CandidateState::Backed,
                                       hasher));
    return add_candidate_entry(std::move(entry));
  }

  bool CandidateStorage::contains(const CandidateHash &candidate_hash) const {
    return by_candidate_hash.contains(candidate_hash);
  }

  Option<std::reference_wrapper<const CandidateEntry>> CandidateStorage::get(
      const CandidateHash &candidate_hash) const {
    if (auto it = by_candidate_hash.find(candidate_hash);
        it != by_candidate_hash.end()) {
      return {{it->second}};
    }
    return std::nullopt;
  }

  Option<std::reference_wrapper<const HeadData>>
  CandidateStorage::head_data_by_hash(const Hash &hash) const {
    auto search = [&](const auto &container)
        -> Option<std::reference_wrapper<const CandidateEntry>> {
      if (auto it = container.find(hash); it != container.end()) {
        if (!it->second.empty()) {
          const CandidateHash &a_candidate = *it->second.begin();
          return get(a_candidate);
        }
      }
      return std::nullopt;
    };

    if (auto e = search(by_output_head)) {
      return {{e->get().candidate->commitments.para_head}};
    }
    if (auto e = search(by_parent_head)) {
      return {{e->get().candidate->persisted_validation_data.parent_head}};
    }
    return std::nullopt;
  }

  void CandidateStorage::remove_candidate(
      const CandidateHash &candidate_hash,
      const std::shared_ptr<crypto::Hasher> &hasher) {
    auto do_remove = [&](HashMap<Hash, HashSet<CandidateHash>> &container,
                         const Hash &target) {
      if (auto it_ = container.find(target); it_ != container.end()) {
        it_->second.erase(candidate_hash);
        if (it_->second.empty()) {
          container.erase(it_);
        }
      }
    };

    auto it = by_candidate_hash.find(candidate_hash);
    if (it != by_candidate_hash.end()) {
      do_remove(by_parent_head, it->second.parent_head_data_hash);
      do_remove(by_output_head, it->second.output_head_data_hash);
      by_candidate_hash.erase(it);
    }
  }

  template <typename F>
  void CandidateStorage::retain(F &&pred /*bool(CandidateHash)*/) {
    for (auto it = by_candidate_hash.begin(); it != by_candidate_hash.end();) {
      if (pred(it->first)) {
        ++it;
      } else {
        it = by_candidate_hash.erase(it);
      }
    }

    auto do_remove = [&](HashMap<Hash, HashSet<CandidateHash>> &container) {
      auto it = container.begin();
      for (; it != container.end();) {
        auto &[_, c] = *it;
        for (auto it_c = c.begin(); it_c != c.end();) {
          if (pred(*it_c)) {
            ++it_c;
          } else {
            it_c = c.erase(it_c);
          }
        }
        if (c.empty()) {
          it = container.erase(it);
        } else {
          ++it;
        }
      }
    };

    do_remove(by_parent_head);
    do_remove(by_output_head);
  }

  void CandidateStorage::mark_backed(const CandidateHash &candidate_hash) {
    if (auto it = by_candidate_hash.find(candidate_hash);
        it != by_candidate_hash.end()) {
      it->second.state = CandidateState::Backed;
    }
  }

  size_t CandidateStorage::len() const {
    return by_candidate_hash.size();
  }

}  // namespace kagome::parachain::fragment
