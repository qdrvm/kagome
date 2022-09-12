/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/availability/store/store_impl.hpp"

namespace kagome::parachain {
  bool AvailabilityStoreImpl::hasChunk(const CandidateHash &candidate_hash,
                                       ValidatorIndex index) {
    auto it = per_candidate_.find(candidate_hash);
    if (it == per_candidate_.end()) {
      return false;
    }
    return it->second.chunks.count(index) != 0;
  }

  bool AvailabilityStoreImpl::hasPov(const CandidateHash &candidate_hash) {
    auto it = per_candidate_.find(candidate_hash);
    if (it == per_candidate_.end()) {
      return false;
    }
    return it->second.pov.has_value();
  }

  bool AvailabilityStoreImpl::hasData(const CandidateHash &candidate_hash) {
    auto it = per_candidate_.find(candidate_hash);
    if (it == per_candidate_.end()) {
      return false;
    }
    return it->second.data.has_value();
  }

  std::optional<AvailabilityStore::ErasureChunk>
  AvailabilityStoreImpl::getChunk(const CandidateHash &candidate_hash,
                                  ValidatorIndex index) {
    auto it = per_candidate_.find(candidate_hash);
    if (it == per_candidate_.end()) {
      return std::nullopt;
    }
    auto it2 = it->second.chunks.find(index);
    if (it2 == it->second.chunks.end()) {
      return std::nullopt;
    }
    return it2->second;
  }

  std::optional<AvailabilityStore::ParachainBlock>
  AvailabilityStoreImpl::getPov(const CandidateHash &candidate_hash) {
    auto it = per_candidate_.find(candidate_hash);
    if (it == per_candidate_.end()) {
      return std::nullopt;
    }
    return it->second.pov;
  }

  void AvailabilityStoreImpl::putChunk(const CandidateHash &candidate_hash,
                                       const ErasureChunk &chunk) {
    per_candidate_[candidate_hash].chunks[chunk.index] = chunk;
  }

  void AvailabilityStoreImpl::putPov(const CandidateHash &candidate_hash,
                                     const ParachainBlock &pov) {
    per_candidate_[candidate_hash].pov = pov;
  }

  void AvailabilityStoreImpl::putData(const CandidateHash &candidate_hash,
                                      const PersistedValidationData &data) {
    per_candidate_[candidate_hash].data = data;
  }
}  // namespace kagome::parachain
