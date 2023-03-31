/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/availability/fetch/fetch_impl.hpp"

#include "parachain/availability/proof.hpp"

namespace kagome::parachain {
  inline auto log() {
    return log::createLogger("FetchImpl", "parachain");
  }

  FetchImpl::FetchImpl(std::shared_ptr<AvailabilityStore> av_store,
                       std::shared_ptr<authority_discovery::Query> query_audi,
                       std::shared_ptr<network::Router> router)
      : av_store_{std::move(av_store)},
        query_audi_{std::move(query_audi)},
        router_{std::move(router)} {
    BOOST_ASSERT(av_store_ != nullptr);
    BOOST_ASSERT(query_audi_ != nullptr);
    BOOST_ASSERT(router_ != nullptr);
  }

  void FetchImpl::fetch(ValidatorIndex chunk_index,
                        const runtime::OccupiedCore &core,
                        const runtime::SessionInfo &session) {
    std::unique_lock lock{mutex_};
    if (active_.find(core.candidate_hash) != active_.end()) {
      return;
    }
    if (av_store_->hasChunk(core.candidate_hash, chunk_index)) {
      return;
    }
    Active active;
    active.chunk_index = chunk_index;
    active.relay_parent = core.candidate_descriptor.relay_parent;
    active.erasure_encoding_root =
        core.candidate_descriptor.erasure_encoding_root;
    for (auto &validator_index :
         session.validator_groups[core.group_responsible]) {
      active.validators.emplace_back(session.discovery_keys[validator_index]);
    }
    active_.emplace(core.candidate_hash, std::move(active));
    lock.unlock();
    fetch(core.candidate_hash);
  }

  void FetchImpl::fetch(const CandidateHash &candidate_hash) {
    std::unique_lock lock{mutex_};
    auto it = active_.find(candidate_hash);
    if (it == active_.end()) {
      return;
    }
    auto &active = it->second;
    while (not active.validators.empty()) {
      auto peer = query_audi_->get(active.validators.back());
      active.validators.pop_back();
      if (peer) {
        router_->getFetchChunkProtocol()->doRequest(
            *peer,
            {candidate_hash, active.chunk_index},
            [=, weak{weak_from_this()}](
                outcome::result<network::FetchChunkResponse> r) {
              if (auto self = weak.lock()) {
                self->fetch(candidate_hash, std::move(r));
              }
            });
        return;
      }
    }
    SL_WARN(log(),
            "candidate={} chunk={} not found",
            candidate_hash,
            active.chunk_index);
    active_.erase(it);
  }

  void FetchImpl::fetch(const CandidateHash &candidate_hash,
                        outcome::result<network::FetchChunkResponse> _chunk) {
    std::unique_lock lock{mutex_};
    auto it = active_.find(candidate_hash);
    if (it == active_.end()) {
      return;
    }
    auto &active = it->second;
    if (_chunk) {
      if (auto chunk2 = boost::get<network::Chunk>(&_chunk.value())) {
        network::ErasureChunk chunk{std::move(chunk2->data),
                                    active.chunk_index,
                                    std::move(chunk2->proof)};
        if (checkTrieProof(chunk, active.erasure_encoding_root)) {
          av_store_->putChunk(
              active.relay_parent, candidate_hash, std::move(chunk));
          SL_VERBOSE(log(),
                     "candidate={} chunk={} fetched",
                     candidate_hash,
                     active.chunk_index);
          active_.erase(it);
          return;
        }
      }
    }
    lock.unlock();
    fetch(candidate_hash);
  }
}  // namespace kagome::parachain
