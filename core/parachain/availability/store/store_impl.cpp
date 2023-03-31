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

  std::optional<AvailabilityStore::AvailableData>
  AvailabilityStoreImpl::getPovAndData(const CandidateHash &candidate_hash) {
    auto it = per_candidate_.find(candidate_hash);
    if (it == per_candidate_.end()) {
      return std::nullopt;
    }
    if (not it->second.pov or not it->second.data) {
      return std::nullopt;
    }
    return AvailableData{*it->second.pov, *it->second.data};
  }

  std::vector<AvailabilityStore::ErasureChunk> AvailabilityStoreImpl::getChunks(
      const CandidateHash &candidate_hash) {
    std::vector<AvailabilityStore::ErasureChunk> chunks;
    auto it = per_candidate_.find(candidate_hash);
    if (it != per_candidate_.end()) {
      for (auto &p : it->second.chunks) {
        chunks.emplace_back(p.second);
      }
    }
    return chunks;
  }

  void AvailabilityStoreImpl::putChunk(const RelayHash &relay_parent,
                                       const CandidateHash &candidate_hash,
                                       ErasureChunk &&chunk) {
    per_candidate_[candidate_hash].chunks[chunk.index] = chunk;
  }

  void AvailabilityStoreImpl::putChunkSet(const CandidateHash &candidate_hash,
                                          std::vector<ErasureChunk> &&chunks) {
    for (auto &&chunk : std::move(chunks)) {
      per_candidate_[candidate_hash].chunks[chunk.index] = std::move(chunk);
    }
  }

  void AvailabilityStoreImpl::putPov(const CandidateHash &candidate_hash,
                                     const ParachainBlock &pov) {
    per_candidate_[candidate_hash].pov = pov;
  }

  void AvailabilityStoreImpl::putData(const CandidateHash &candidate_hash,
                                      const PersistedValidationData &data) {
    per_candidate_[candidate_hash].data = data;
  }

  void AvailabilityStoreImpl::registerCandidate(
      network::RelayHash const &relay_parent,
      CandidateHash const &candidate_hash) {
    candidates_[relay_parent].insert(candidate_hash);
  }

  void AvailabilityStoreImpl::remove(network::RelayHash const &relay_parent) {
    if (auto it = candidates_.find(relay_parent); it != candidates_.end()) {
      for (auto const &l : it->second) {
        per_candidate_.erase(l);
      }
      candidates_.erase(it);
    }
  }
}  // namespace kagome::parachain
