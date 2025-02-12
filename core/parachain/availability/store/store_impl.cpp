/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/availability/store/store_impl.hpp"
#include "candidate_chunk_key.hpp"

constexpr uint64_t KEEP_CANDIDATES_TIMEOUT = 1 * 60;

namespace kagome::parachain {
  AvailabilityStoreImpl::AvailabilityStoreImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      clock::SteadyClock &steady_clock,
      std::shared_ptr<storage::SpacedStorage> storage,
      primitives::events::ChainSubscriptionEnginePtr chain_sub_engine)
      : steady_clock_{steady_clock},
        storage_{std::move(storage)},
        chain_sub_{std::move(chain_sub_engine)} {
    BOOST_ASSERT(storage_ != nullptr);

    app_state_manager->takeControl(*this);
  }

  bool AvailabilityStoreImpl::start() {
    chain_sub_.onDeactivate(
        [weak{weak_from_this()}](
            const primitives::events::RemoveAfterFinalizationParams &params) {
          auto self = weak.lock();
          if (not self) {
            return;
          }
          if (params.removed.empty()) {
            return;
          }
          self->state_.exclusiveAccess([self, &params](auto &state) {
            for (const auto &header_info : params.removed) {
              self->remove_no_lock(state, header_info.hash);
            }
          });
        });
    return true;
  }

  bool AvailabilityStoreImpl::hasChunk(const CandidateHash &candidate_hash,
                                       ValidatorIndex index) const {
    const auto has_chunk = state_.sharedAccess([&](const auto &state) {
      auto it = state.per_candidate_.find(candidate_hash);
      if (it == state.per_candidate_.end()) {
        return false;
      }
      return it->second.chunks.count(index) != 0;
    });
    if (has_chunk) {
      return true;
    }
    const auto space = storage_->getSpace(storage::Space::kAvaliabilityStorage);
    if (not space) {
      SL_CRITICAL(logger,
                  "Failed to get AvaliabilityStorage space in hasChunk");
      return false;
    }
    auto chunk_from_db =
        space->get(CandidateChunkKey::encode(candidate_hash, index));
    return chunk_from_db.has_value();
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
    auto chunk = state_.sharedAccess(
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
    if (chunk) {
      return chunk;
    }
    auto space = storage_->getSpace(storage::Space::kAvaliabilityStorage);
    if (not space) {
      SL_ERROR(logger, "Failed to get space for candidate {}", candidate_hash);
      return std::nullopt;
    }
    auto chunk_from_db =
        space->get(CandidateChunkKey::encode(candidate_hash, index));
    if (not chunk_from_db) {
      return std::nullopt;
    }
    const auto decoded_chunk =
        scale::decode<ErasureChunk>(chunk_from_db.value());
    if (not decoded_chunk) {
      SL_ERROR(logger,
               "Failed to decode chunk candidate {} index {} error {}",
               candidate_hash,
               index,
               decoded_chunk.error());
      return std::nullopt;
    }
    return decoded_chunk.value();
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
    auto chunks = state_.sharedAccess([&](const auto &state) {
      std::vector<AvailabilityStore::ErasureChunk> chunks;
      auto it = state.per_candidate_.find(candidate_hash);
      if (it != state.per_candidate_.end()) {
        for (auto &p : it->second.chunks) {
          chunks.emplace_back(p.second);
        }
      }
      return chunks;
    });
    if (not chunks.empty()) {
      return chunks;
    }
    auto space = storage_->getSpace(storage::Space::kAvaliabilityStorage);
    if (not space) {
      SL_CRITICAL(logger,
                  "Failed to get AvaliabilityStorage space in getChunks");
      return chunks;
    }
    auto cursor = space->cursor();
    if (not cursor) {
      SL_ERROR(logger, "Failed to get cursor for AvaliabilityStorage");
      return chunks;
    }
    const auto seek_key = CandidateChunkKey::encode_hash(candidate_hash);
    auto seek_res = cursor->seek(seek_key);
    if (not seek_res) {
      SL_ERROR(logger,
               "Failed to seek for candidate {} error: {}",
               candidate_hash,
               seek_res.error());
      return chunks;
    }
    if (not seek_res.value()) {
      SL_DEBUG(logger, "Seek not found for candidate {}", candidate_hash);
      return chunks;
    }
    const auto check_key = [&seek_key](const auto &key) {
      if (not key) {
        return false;
      }
      const auto &key_value = key.value();
      return key_value.size() >= seek_key.size()
         and std::equal(seek_key.begin(), seek_key.end(), key_value.begin());
    };
    while (cursor->isValid() and check_key(cursor->key())) {
      const auto cursor_opt_value = cursor->value();
      if (cursor_opt_value) {
        auto decoded_res =
            scale::decode<ErasureChunk>(cursor_opt_value.value());
        if (decoded_res) {
          chunks.emplace_back(std::move(decoded_res.value()));
        } else {
          SL_ERROR(logger,
                   "Failed to decode value for candidate hash {} error: {}",
                   candidate_hash,
                   decoded_res.error());
        }
      } else {
        SL_ERROR(logger,
                 "Failed to get value candidate {} for key {}",
                 candidate_hash,
                 cursor->key()->toHex());
      }
      if (not cursor->next()) {
        break;
      }
    }
    return chunks;
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
    SL_TRACE(logger, "Attempt to store all chunks of {}", candidate_hash);

    state_.exclusiveAccess([&](auto &state) {
      prune_candidates_no_lock(state);
      state.candidates_[relay_parent].insert(candidate_hash);
      auto &candidate_data = state.per_candidate_[candidate_hash];
      for (auto &&chunk : std::move(chunks)) {
        auto encoded_chunk = scale::encode(chunk);
        const auto chunk_index = chunk.index;
        candidate_data.chunks[chunk.index] = std::move(chunk);
        if (not encoded_chunk) {
          SL_ERROR(logger,
                   "Failed to encode chunk, error: {}",
                   encoded_chunk.error());
          continue;
        }
        auto space = storage_->getSpace(storage::Space::kAvaliabilityStorage);
        if (not space) {
          SL_ERROR(logger, "Failed to get space");
          continue;
        }
        if (auto res = space->put(
                CandidateChunkKey::encode(candidate_hash, chunk_index),
                std::move(encoded_chunk.value()));
            not res) {
          SL_ERROR(logger,
                   "Failed to put chunk candidate {} index {} error {}",
                   candidate_hash,
                   chunk_index,
                   res.error());
        } else {
          SL_TRACE(logger,
                  "Chunk {}:{} is saved by storeData()",
                  candidate_hash,
                  chunk.index);
        }
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
    SL_TRACE(logger, "Attempt to put chunk {}:{}", candidate_hash, chunk.index);

    auto encoded_chunk = scale::encode(chunk);
    const auto chunk_index = chunk.index;
    state_.exclusiveAccess([&](auto &state) {
      prune_candidates_no_lock(state);
      state.candidates_[relay_parent].insert(candidate_hash);
      state.per_candidate_[candidate_hash].chunks[chunk.index] =
          std::move(chunk);
      state.candidates_living_keeper_.emplace_back(steady_clock_.nowUint64(),
                                                   relay_parent);
    });
    if (not encoded_chunk) {
      SL_ERROR(
          logger, "Failed to encode chunk, error: {}", encoded_chunk.error());
      return;
    }

    auto space = storage_->getSpace(storage::Space::kAvaliabilityStorage);
    if (not space) {
      SL_ERROR(logger, "Failed to get AvaliabilityStorage space");
      return;
    }

    if (auto res =
            space->put(CandidateChunkKey::encode(candidate_hash, chunk_index),
                       std::move(encoded_chunk.value()));
        not res) {
      SL_ERROR(logger,
               "Failed to put chunk candidate {} index {} error {}",
               candidate_hash,
               chunk_index,
               res.error());
    }

    SL_TRACE(logger,
            "Chunk {}:{} is saved by putChunk()",
            candidate_hash,
            chunk.index);
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
