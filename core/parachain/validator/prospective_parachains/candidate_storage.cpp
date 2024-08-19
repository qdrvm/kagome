/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/prospective_parachains/candidate_storage.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::parachain::fragment,
                            CandidateStorage::Error,
                            e) {
  using E = kagome::parachain::fragment::CandidateStorage::Error;
  switch (e) {
    case E::CANDIDATE_ALREADY_KNOWN:
      return "Candidate already known";
    case E::PERSISTED_VALIDATION_DATA_MISMATCH:
      return "Persisted validation data mismatch";
  }
  return "Unknown error";
}

namespace kagome::parachain::fragment {

  outcome::result<void> CandidateStorage::addCandidate(
      const CandidateHash &candidate_hash,
      const network::CommittedCandidateReceipt &candidate,
      const crypto::Hashed<const runtime::PersistedValidationData &,
                           32,
                           crypto::Blake2b_StreamHasher<32>>
          &persisted_validation_data,
      const std::shared_ptr<crypto::Hasher> &hasher) {
    if (by_candidate_hash.contains(candidate_hash)) {
      return Error::CANDIDATE_ALREADY_KNOWN;
    }

    if (persisted_validation_data.getHash()
        != candidate.descriptor.persisted_data_hash) {
      return Error::PERSISTED_VALIDATION_DATA_MISMATCH;
    }

    const auto parent_head_hash =
        hasher->blake2b_256(persisted_validation_data.get().parent_head);
    const auto output_head_hash =
        hasher->blake2b_256(candidate.commitments.para_head);

    by_parent_head[parent_head_hash].insert(candidate_hash);
    by_output_head[output_head_hash].insert(candidate_hash);

    by_candidate_hash.insert(
        {candidate_hash,
         CandidateEntry{
             .candidate_hash = candidate_hash,
             .relay_parent = candidate.descriptor.relay_parent,
             .parent_head_data_hash = parent_head_hash,
             .output_head_data_hash = output_head_hash,
             .candidate =
                 ProspectiveCandidate{
                     .commitments = candidate.commitments,
                     .collator = candidate.descriptor.collator_id,
                     .collator_signature = candidate.descriptor.signature,
                     .persisted_validation_data =
                         persisted_validation_data.get(),
                     .pov_hash = candidate.descriptor.pov_hash,
                     .validation_code_hash =
                         candidate.descriptor.validation_code_hash},
             .state = CandidateState::Introduced,
         }});
    return outcome::success();
  }

  Option<std::reference_wrapper<const CandidateEntry>> CandidateStorage::get(
      const CandidateHash &candidate_hash) const {
    if (auto it = by_candidate_hash.find(candidate_hash);
        it != by_candidate_hash.end()) {
      return {{it->second}};
    }
    return std::nullopt;
  }

  Option<Hash> CandidateStorage::relayParentByCandidateHash(
      const CandidateHash &candidate_hash) const {
    if (auto it = by_candidate_hash.find(candidate_hash);
        it != by_candidate_hash.end()) {
      return it->second.relay_parent;
    }
    return std::nullopt;
  }

  bool CandidateStorage::contains(const CandidateHash &candidate_hash) const {
    return by_candidate_hash.contains(candidate_hash);
  }

  template <typename F>
  void CandidateStorage::possibleParaChildren(const Hash &parent_head_hash,
                                              F &&func) const {
    if (auto it = by_parent_head.find(parent_head_hash);
        it != by_parent_head.end()) {
      for (const auto &h : it->second) {
        if (auto c_it = by_candidate_hash.find(h);
            c_it != by_candidate_hash.end()) {
          std::forward<F>(func)(c_it->second);
        }
      }
    }
  }

  Option<std::reference_wrapper<const HeadData>>
  CandidateStorage::headDataByHash(const Hash &hash) const {
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
      return {{e->get().candidate.commitments.para_head}};
    }
    if (auto e = search(by_parent_head)) {
      return {{e->get().candidate.persisted_validation_data.parent_head}};
    }
    return std::nullopt;
  }

  void CandidateStorage::removeCandidate(
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

  void CandidateStorage::markSeconded(const CandidateHash &candidate_hash) {
    if (auto it = by_candidate_hash.find(candidate_hash);
        it != by_candidate_hash.end()) {
      if (it->second.state != CandidateState::Backed) {
        it->second.state = CandidateState::Seconded;
      }
    }
  }

  void CandidateStorage::markBacked(const CandidateHash &candidate_hash) {
    if (auto it = by_candidate_hash.find(candidate_hash);
        it != by_candidate_hash.end()) {
      it->second.state = CandidateState::Backed;
    }
  }

  bool CandidateStorage::isBacked(const CandidateHash &candidate_hash) const {
    auto it = by_candidate_hash.find(candidate_hash);
    return it != by_candidate_hash.end()
        && it->second.state == CandidateState::Backed;
  }

  std::pair<size_t, size_t> CandidateStorage::len() const {
    return std::make_pair(by_parent_head.size(), by_candidate_hash.size());
  }

}  // namespace kagome::parachain::fragment
