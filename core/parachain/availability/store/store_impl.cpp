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
    SL_TRACE(
        logger, "hasChunk candidate_hash={} index={}", candidate_hash, index);
    const auto has_chunk = state_.sharedAccess([&](const auto &state) {
      auto it = state.per_candidate_.find(candidate_hash);
      if (it == state.per_candidate_.end()) {
        return false;
      }
      return it->second.chunks.count(index) != 0;
    });
    if (has_chunk) {
      SL_TRACE(logger,
               "hasChunk found in cache candidate_hash={} index={} true",
               candidate_hash,
               index);
      return true;
    }
    SL_TRACE(logger,
             "hasChunk not found in cache candidate_hash={} index={}",
             candidate_hash,
             index);
    const auto space = storage_->getSpace(storage::Space::kAvaliabilityStorage);
    if (not space) {
      SL_CRITICAL(logger,
                  "Failed to get AvaliabilityStorage space in hasChunk");
      return false;
    }
    auto chunk_from_db =
        space->get(CandidateChunkKey::encode(candidate_hash, index));
    if (chunk_from_db.has_value()) {
      SL_TRACE(logger,
               "hasChunk found in db candidate_hash={} index={}",
               candidate_hash,
               index);
    } else {
      SL_DEBUG(logger,
               "hasChunk not found in db candidate_hash={} index={}",
               candidate_hash,
               index);
    }
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
    SL_TRACE(
        logger, "getChunk candidate_hash={} index={}", candidate_hash, index);
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
      SL_TRACE(logger,
               "getChunk found in cache candidate_hash={} index={}",
               candidate_hash,
               index);
      return chunk;
    }
    SL_TRACE(logger,
             "getChunk not found in cache candidate_hash={} index={}",
             candidate_hash,
             index);
    auto space = storage_->getSpace(storage::Space::kAvaliabilityStorage);
    if (not space) {
      SL_ERROR(logger, "Failed to get space for candidate {}", candidate_hash);
      return std::nullopt;
    }
    auto chunk_from_db =
        space->get(CandidateChunkKey::encode(candidate_hash, index));
    if (not chunk_from_db) {
      SL_TRACE(logger,
               "getChunk not found in db candidate_hash={} index={}",
               candidate_hash,
               index);
      return std::nullopt;
    }
    SL_TRACE(logger,
             "getChunk found in db candidate_hash={} index={}",
             candidate_hash,
             index);
    const auto decoded_chunk =
        scale::decode<ErasureChunk>(chunk_from_db.value());
    if (not decoded_chunk) {
      SL_ERROR(logger,
               "Failed to decode chunk for candidate {} index {} error {}",
               candidate_hash,
               index,
               decoded_chunk.error());
      return std::nullopt;
    }
    SL_TRACE(logger,
             "getChunk decoded candidate_hash={} index={}",
             candidate_hash,
             index);
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
    SL_TRACE(logger, "getChunks candidate_hash={}", candidate_hash);
    auto chunks = state_.sharedAccess([&](const auto &state) {
      std::vector<AvailabilityStore::ErasureChunk> chunks;
      auto it = state.per_candidate_.find(candidate_hash);
      if (it != state.per_candidate_.end()) {
        for (auto &chunk : it->second.chunks | std::views::values) {
          chunks.emplace_back(chunk);
          SL_TRACE(logger,
                   "getChunks found in cache candidate_hash={} index={}",
                   candidate_hash,
                   chunk.index);
        }
      }
      return chunks;
    });
    if (not chunks.empty()) {
      SL_TRACE(logger,
               "getChunks found {} chunks in cache candidate_hash={}",
               chunks.size(),
               candidate_hash);
      return chunks;
    }
    SL_TRACE(logger,
             "getChunks not found in cache candidate_hash={}",
             candidate_hash);
    auto space = storage_->getSpace(storage::Space::kAvaliabilityStorage);
    if (not space) {
      SL_CRITICAL(logger,
                  "Failed to get AvaliabilityStorage space in getChunks");
      return chunks;
    }
    auto cursor = space->cursor();
    if (not cursor) {
      SL_ERROR(logger,
               "Failed to get cursor for AvaliabilityStorage for candidate {}",
               candidate_hash);
      return chunks;
    }
    SL_TRACE(logger,
             "getChunks got cursor for AvaliabilityStorage for candidate {}",
             candidate_hash);
    const auto seek_key = CandidateChunkKey::encode_hash(candidate_hash);
    auto seek_res = cursor->seek(seek_key);
    if (not seek_res) {
      SL_ERROR(logger,
               "Failed to seek for candidate {} error: {}",
               candidate_hash,
               seek_res.error());
      return chunks;
    }
    SL_TRACE(logger, "getChunks seeked for candidate {}", candidate_hash);
    if (not seek_res.value()) {
      SL_DEBUG(logger, "Seek not found for candidate {}", candidate_hash);
      return chunks;
    }
    SL_TRACE(logger, "Seek found for candidate {}", candidate_hash);
    const auto check_key = [&seek_key, &candidate_hash, this](const auto &key) {
      if (not key) {
        SL_ERROR(logger, "Key is null for candidate {}", candidate_hash);
        return false;
      }
      const auto &key_value = key.value();
      SL_DEBUG(logger,
               "Checking key for candidate {}: size={}, seek_key size={}",
               candidate_hash,
               key_value.size(),
               seek_key.size());

      bool size_check = key_value.size() >= seek_key.size();
      if (!size_check) {
        SL_DEBUG(logger,
                 "Key size check failed for candidate {}: key size {} < "
                 "seek_key size {}",
                 candidate_hash,
                 key_value.size(),
                 seek_key.size());
        return false;
      }

      bool prefix_check =
          std::equal(seek_key.begin(), seek_key.end(), key_value.begin());
      SL_DEBUG(logger,
               "Key prefix check for candidate {}: result={}",
               candidate_hash,
               prefix_check);

      return size_check && prefix_check;
    };
    while (cursor->isValid()) {
      SL_DEBUG(
          logger, "getChunks cursor is valid for candidate {}", candidate_hash);
      if (not check_key(cursor->key())) {
        SL_DEBUG(logger,
                 "getChunks key check failed for candidate {}, breaking",
                 candidate_hash);
        break;
      }
      SL_DEBUG(logger,
               "getChunks key check passed for candidate {}, getting value",
               candidate_hash);
      const auto cursor_opt_value = cursor->value();
      if (cursor_opt_value) {
        auto decoded_res =
            scale::decode<ErasureChunk>(cursor_opt_value.value());
        if (decoded_res) {
          chunks.emplace_back(std::move(decoded_res.value()));
          SL_TRACE(logger,
                   "getChunks found chunk candidate_hash={} index={}",
                   candidate_hash,
                   chunks.back().index);
        } else {
          SL_ERROR(logger,
                   "Failed to decode value for candidate hash {} error: {}",
                   candidate_hash,
                   decoded_res.error());
        }
      } else {
        SL_ERROR(logger,
                 "Failed to get value for candidate {} for key {}",
                 candidate_hash,
                 cursor->key()->toHex());
      }
      if (not cursor->next()) {
        SL_TRACE(
            logger, "getChunks next is false for candidate {}", candidate_hash);
        break;
      }
    }
    SL_TRACE(logger,
             "getChunks found {} chunks for candidate {}",
             chunks.size(),
             candidate_hash);
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
    SL_TRACE(logger,
             "storeData candidate_hash={} chunks_count={}",
             candidate_hash,
             chunks.size());

    state_.exclusiveAccess([&](auto &state) {
      SL_TRACE(logger,
               "storeData acquired exclusive access for candidate {}",
               candidate_hash);
      prune_candidates_no_lock(state);
      state.candidates_[relay_parent].insert(candidate_hash);
      SL_TRACE(logger,
               "storeData added candidate {} to relay_parent={}",
               candidate_hash,
               relay_parent);

      auto &candidate_data = state.per_candidate_[candidate_hash];
      SL_TRACE(logger,
               "storeData processing chunks for candidate {}",
               candidate_hash);

      for (auto &&chunk : std::move(chunks)) {
        SL_TRACE(logger,
                 "storeData processing chunk index={} for candidate {}",
                 chunk.index,
                 candidate_hash);

        auto encoded_chunk = scale::encode(chunk);
        const auto chunk_index = chunk.index;
        candidate_data.chunks[chunk.index] = std::move(chunk);
        SL_TRACE(logger,
                 "storeData added chunk to state candidate_hash={} index={}",
                 candidate_hash,
                 chunk_index);

        if (not encoded_chunk) {
          SL_ERROR(
              logger,
              "storeData Failed to encode chunk for candidate {}, error: {}",
              candidate_hash,
              encoded_chunk.error());
          continue;
        }
        SL_TRACE(logger,
                 "storeData encoded chunk candidate_hash={} index={}",
                 candidate_hash,
                 chunk_index);

        auto space = storage_->getSpace(storage::Space::kAvaliabilityStorage);
        if (not space) {
          SL_ERROR(logger,
                   "storeData Failed to get AvaliabilityStorage space for "
                   "candidate {}",
                   candidate_hash);
          continue;
        }
        SL_TRACE(logger,
                 "storeData got storage space for candidate {}",
                 candidate_hash);

        if (auto res = space->put(
                CandidateChunkKey::encode(candidate_hash, chunk_index),
                std::move(encoded_chunk.value()));
            not res) {
          SL_ERROR(
              logger,
              "storeData Failed to put chunk candidate {} index {} error {}",
              candidate_hash,
              chunk_index,
              res.error());
        } else {
          SL_TRACE(logger,
                   "storeData storeData chunk candidate {} index {} to store",
                   candidate_hash,
                   chunk_index);
        }
      }

      candidate_data.pov = pov;
      candidate_data.data = data;
      SL_TRACE(logger,
               "storeData saved pov and data for candidate_hash={}",
               candidate_hash);

      state.candidates_living_keeper_.emplace_back(steady_clock_.nowUint64(),
                                                   relay_parent);
      SL_TRACE(logger,
               "storeData updated candidates_living_keeper for relay_parent={}",
               relay_parent);
    });

    SL_TRACE(
        logger, "storeData completed for candidate_hash={}", candidate_hash);
  }

  void AvailabilityStoreImpl::putChunk(const network::RelayHash &relay_parent,
                                       const CandidateHash &candidate_hash,
                                       ErasureChunk &&chunk) {
    SL_TRACE(logger,
             "putChunk candidate_hash={} index={}",
             candidate_hash,
             chunk.index);

    auto encoded_chunk = scale::encode(chunk);
    const auto chunk_index = chunk.index;
    state_.exclusiveAccess([&](auto &state) {
      prune_candidates_no_lock(state);
      state.candidates_[relay_parent].insert(candidate_hash);
      state.per_candidate_[candidate_hash].chunks[chunk.index] =
          std::move(chunk);
      state.candidates_living_keeper_.emplace_back(steady_clock_.nowUint64(),
                                                   relay_parent);
      SL_TRACE(logger,
               "putChunk to state candidate_hash={} index={}",
               candidate_hash,
               chunk.index);
    });
    if (not encoded_chunk) {
      SL_ERROR(logger,
               "putChunk Failed to encode chunk for candidate {}, error: {}",
               candidate_hash,
               encoded_chunk.error());
      return;
    }
    SL_TRACE(logger,
             "putChunk encoded candidate_hash={} index={}",
             candidate_hash,
             chunk.index);
    auto space = storage_->getSpace(storage::Space::kAvaliabilityStorage);
    if (not space) {
      SL_ERROR(
          logger,
          "putChunk Failed to get AvaliabilityStorage space for candidate {}",
          candidate_hash);
      return;
    }

    if (auto res =
            space->put(CandidateChunkKey::encode(candidate_hash, chunk_index),
                       std::move(encoded_chunk.value()));
        not res) {
      SL_ERROR(logger,
               "Failed to put chunk for candidate {} index {} error {}",
               candidate_hash,
               chunk_index,
               res.error());
    }

    SL_TRACE(logger,
             "putChunk candidate {} index {} saved to store",
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
