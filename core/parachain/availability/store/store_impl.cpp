/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/availability/store/store_impl.hpp"

namespace kagome::parachain {
  bool AvailabilityStoreImpl::hasChunk(const CandidateHash &candidate_hash,
                                       ValidatorIndex index) const {
    return state_.sharedAccess([&](auto const &state) {
      auto it = state.per_candidate_.find(candidate_hash);
      if (it == state.per_candidate_.end()) {
        return false;
      }
      return it->second.chunks.count(index) != 0;
    });
  }

  bool AvailabilityStoreImpl::hasPov(
      const CandidateHash &candidate_hash) const {
    return state_.sharedAccess([&](auto const &state) {
      auto it = state.per_candidate_.find(candidate_hash);
      if (it == state.per_candidate_.end()) {
        return false;
      }
      return it->second.pov.has_value();
    });
  }

  bool AvailabilityStoreImpl::hasData(
      const CandidateHash &candidate_hash) const {
    return state_.sharedAccess([&](auto const &state) {
      auto it = state.per_candidate_.find(candidate_hash);
      if (it == state.per_candidate_.end()) {
        return false;
      }
      return it->second.data.has_value();
    });
  }

  std::optional<AvailabilityStore::ErasureChunk>
  AvailabilityStoreImpl::getChunk(const CandidateHash &candidate_hash,
                                  ValidatorIndex index) const {
    return state_.sharedAccess(
        [&](auto const &state)
            -> std::optional<AvailabilityStore::ErasureChunk> {
          auto it = state.per_candidate_.find(candidate_hash);
          if (it == state.per_candidate_.end()) {
            return std::nullopt;
          }
          auto it2 = it->second.chunks.find(index);
          if (it2 == it->second.chunks.end()) {
            return std::nullopt;
          }
          return it2->second;
        });
  }

  std::optional<AvailabilityStore::ParachainBlock>
  AvailabilityStoreImpl::getPov(const CandidateHash &candidate_hash) const {
    return state_.sharedAccess(
        [&](auto const &state)
            -> std::optional<AvailabilityStore::ParachainBlock> {
          auto it = state.per_candidate_.find(candidate_hash);
          if (it == state.per_candidate_.end()) {
            return std::nullopt;
          }
          return it->second.pov;
        });
  }

  std::optional<AvailabilityStore::AvailableData>
  AvailabilityStoreImpl::getPovAndData(
      const CandidateHash &candidate_hash) const {
    return state_.sharedAccess(
        [&](auto const &state)
            -> std::optional<AvailabilityStore::AvailableData> {
          auto it = state.per_candidate_.find(candidate_hash);
          if (it == state.per_candidate_.end()) {
            return std::nullopt;
          }
          if (not it->second.pov or not it->second.data) {
            return std::nullopt;
          }
          return AvailableData{*it->second.pov, *it->second.data};
        });
  }

  std::vector<AvailabilityStore::ErasureChunk> AvailabilityStoreImpl::getChunks(
      const CandidateHash &candidate_hash) const {
    return state_.sharedAccess([&](auto const &state) {
      std::vector<AvailabilityStore::ErasureChunk> chunks;
      auto it = state.per_candidate_.find(candidate_hash);
      if (it != state.per_candidate_.end()) {
        for (auto &p : it->second.chunks) {
          chunks.emplace_back(p.second);
        }
      }
      return chunks;
    });
  }

  void AvailabilityStoreImpl::storeData(network::RelayHash const &relay_parent,
                                        CandidateHash const &candidate_hash,
                                        std::vector<ErasureChunk> &&chunks,
                                        ParachainBlock const &pov,
                                        PersistedValidationData const &data) {
    state_.exclusiveAccess([&](auto &state) {
      state.candidates_[relay_parent].insert(candidate_hash);
      auto &candidate_data = state.per_candidate_[candidate_hash];
      for (auto &&chunk : std::move(chunks)) {
        candidate_data.chunks[chunk.index] = std::move(chunk);
      }
      candidate_data.pov = pov;
      candidate_data.data = data;
    });
  }

  void AvailabilityStoreImpl::putChunk(network::RelayHash const &relay_parent, const CandidateHash &candidate_hash,
                                       ErasureChunk &&chunk) {
    state_.exclusiveAccess([&](auto &state) {
      state.candidates_[relay_parent].insert(candidate_hash);
      state.per_candidate_[candidate_hash].chunks[chunk.index] = std::move(chunk);
    });
  }

  void AvailabilityStoreImpl::remove(network::RelayHash const &relay_parent) {
    state_.exclusiveAccess([&](auto &state) {
      if (auto it = state.candidates_.find(relay_parent);
          it != state.candidates_.end()) {
        for (auto const &l : it->second) {
          state.per_candidate_.erase(l);
        }
        state.candidates_.erase(it);
      }
    });
  }
}  // namespace kagome::parachain
