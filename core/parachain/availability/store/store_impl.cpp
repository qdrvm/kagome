/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/availability/store/store_impl.hpp"

constexpr uint64_t KEEP_CANDIDATES_TIMEOUT = 1 * 60;

namespace kagome::parachain {
  AvailabilityStoreImpl::AvailabilityStoreImpl(clock::SteadyClock &steady_clock)
      : steady_clock_(steady_clock) {}

  bool AvailabilityStoreImpl::hasChunk(const CandidateHash &candidate_hash,
                                       ValidatorIndex index) const {
    return state_.sharedAccess([&](const auto &state) {
      auto it = state.per_candidate_.find(candidate_hash);
      if (it == state.per_candidate_.end()) {
        return false;
      }
      return it->second.chunks.count(index) != 0;
    });
  }

  bool AvailabilityStoreImpl::hasPov(
      const CandidateHash &candidate_hash) const {
    return state_.sharedAccess([&](const auto &state) {
      auto it = state.per_candidate_.find(candidate_hash);
      if (it == state.per_candidate_.end()) {
        return false;
      }
      return it->second.pov.has_value();
    });
  }

  bool AvailabilityStoreImpl::hasData(
      const CandidateHash &candidate_hash) const {
    return state_.sharedAccess([&](const auto &state) {
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
        [&](const auto &state)
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
        [&](const auto &state)
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
        [&](const auto &state)
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
    return state_.sharedAccess([&](const auto &state) {
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

  void AvailabilityStoreImpl::printStoragesLoad() {
    state_.sharedAccess([&](auto &state) {
      SL_TRACE(logger,
               "[Availability store statistics]:"
               "\n\t-> state.candidates={}"
               "\n\t-> state.per_candidate={}",
               state.candidates_.size(),
               state.per_candidate_.size());
    });
  }

  void AvailabilityStoreImpl::prune_candidates_no_lock(State &state) {
    const auto now = steady_clock_.nowUint64();
    while (!state.candidates_living_keeper_.empty()
           && state.candidates_living_keeper_[0].first + KEEP_CANDIDATES_TIMEOUT
                  < now) {
      remove_no_lock(state, state.candidates_living_keeper_[0].second);
      state.candidates_living_keeper_.pop_front();
    }
  }

  void AvailabilityStoreImpl::storeData(const network::RelayHash &relay_parent,
                                        const CandidateHash &candidate_hash,
                                        std::vector<ErasureChunk> &&chunks,
                                        const ParachainBlock &pov,
                                        const PersistedValidationData &data) {
    state_.exclusiveAccess([&](auto &state) {
      prune_candidates_no_lock(state);
      state.candidates_[relay_parent].insert(candidate_hash);
      auto &candidate_data = state.per_candidate_[candidate_hash];
      for (auto &&chunk : std::move(chunks)) {
        candidate_data.chunks[chunk.index] = std::move(chunk);
      }
      candidate_data.pov = pov;
      candidate_data.data = data;
      state.candidates_living_keeper_.emplace_back(steady_clock_.nowUint64(),
                                                   relay_parent);
    });
  }

  void AvailabilityStoreImpl::putChunk(const network::RelayHash &relay_parent,
                                       const CandidateHash &candidate_hash,
                                       ErasureChunk &&chunk) {
    state_.exclusiveAccess([&](auto &state) {
      prune_candidates_no_lock(state);
      state.candidates_[relay_parent].insert(candidate_hash);
      state.per_candidate_[candidate_hash].chunks[chunk.index] =
          std::move(chunk);
      state.candidates_living_keeper_.emplace_back(steady_clock_.nowUint64(),
                                                   relay_parent);
    });
  }

  void AvailabilityStoreImpl::remove_no_lock(
      State &state, const network::RelayHash &relay_parent) {
    if (auto it = state.candidates_.find(relay_parent);
        it != state.candidates_.end()) {
      for (const auto &l : it->second) {
        state.per_candidate_.erase(l);
      }
      state.candidates_.erase(it);
    }
  }

  void AvailabilityStoreImpl::remove(const network::RelayHash &relay_parent) {
    state_.exclusiveAccess(
        [&](auto &state) { remove_no_lock(state, relay_parent); });
  }
}  // namespace kagome::parachain
