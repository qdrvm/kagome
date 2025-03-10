/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/availability/fetch/fetch_impl.hpp"

#include "authority_discovery/query/query.hpp"
#include "network/impl/protocols/protocol_fetch_chunk.hpp"
#include "network/impl/protocols/protocol_fetch_chunk_obsolete.hpp"
#include "network/peer_manager.hpp"
#include "network/router.hpp"
#include "parachain/availability/proof.hpp"
#include "parachain/availability/store/store.hpp"

namespace kagome::parachain {
  inline auto log() {
    return log::createLogger("Fetch", "parachain");
  }

  FetchImpl::FetchImpl(std::shared_ptr<AvailabilityStore> av_store,
                       std::shared_ptr<authority_discovery::Query> query_audi,
                       std::shared_ptr<network::Router> router,
                       std::shared_ptr<network::PeerManager> pm)
      : av_store_{std::move(av_store)},
        query_audi_{std::move(query_audi)},
        router_{std::move(router)},
        pm_{std::move(pm)} {
    BOOST_ASSERT(av_store_ != nullptr);
    BOOST_ASSERT(query_audi_ != nullptr);
    BOOST_ASSERT(router_ != nullptr);
    BOOST_ASSERT(pm_ != nullptr);
  }

  void FetchImpl::fetch(ChunkIndex chunk_index,
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

      if (not peer.has_value()) {
        continue;
      }
      const auto &peer_id = peer.value().id;

      auto peer_state = [&]() {
        auto res = pm_->getPeerState(peer_id);
        if (!res) {
          SL_TRACE(log(),
                   "No PeerState of peer {}. Default one has created",
                   peer_id);
          res = pm_->createDefaultPeerState(peer_id);
        }
        return res;
      }();

      auto req_chunk_version = peer_state->get().req_chunk_version.value_or(
          network::ReqChunkVersion::V2);

      switch (req_chunk_version) {
        case network::ReqChunkVersion::V2:
          SL_DEBUG(log(),
                   "Sent request of chunk {} of candidate {} to peer {}",
                   active.chunk_index,
                   candidate_hash,
                   peer_id);
          router_->getFetchChunkProtocol()->doRequest(
              peer_id,
              {candidate_hash, active.chunk_index},
              [=, chunk_index{active.chunk_index}, weak{weak_from_this()}](
                  outcome::result<network::FetchChunkResponse> r) {
                if (auto self = weak.lock()) {
                  if (r.has_value()) {
                    SL_DEBUG(log(),
                             "Result of request chunk {} of candidate {} to "
                             "peer {}: success",
                             chunk_index,
                             candidate_hash,
                             peer_id);
                  } else {
                    SL_DEBUG(log(),
                             "Result of request chunk {} of candidate {} to "
                             "peer {}: {}",
                             chunk_index,
                             candidate_hash,
                             peer_id,
                             r.error());
                  }

                  self->fetch(candidate_hash, std::move(r));
                }
              });
          break;
        case network::ReqChunkVersion::V1_obsolete:
          router_->getFetchChunkProtocolObsolete()->doRequest(
              peer_id,
              {candidate_hash, active.chunk_index},
              [=, weak{weak_from_this()}](
                  outcome::result<network::FetchChunkResponseObsolete> r) {
                if (auto self = weak.lock()) {
                  if (r.has_value()) {
                    auto response = visit_in_place(
                        r.value(),
                        [](network::Empty &empty)
                            -> network::FetchChunkResponse { return empty; },
                        [&](network::ChunkObsolete &chunk_obsolete)
                            -> network::FetchChunkResponse {
                          return network::Chunk{
                              .data = std::move(chunk_obsolete.data),
                              .chunk_index = active.chunk_index,
                              .proof = std::move(chunk_obsolete.proof),
                          };
                        });
                    self->fetch(candidate_hash, std::move(response));
                  } else {
                    self->fetch(candidate_hash, r.as_failure());
                  }
                }
              });
          break;
        default:
          UNREACHABLE;
      }
      return;
    }
    SL_DEBUG(log(),
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
        network::ErasureChunk chunk{
            .chunk = std::move(chunk2->data),
            .index = active.chunk_index,
            .proof = std::move(chunk2->proof),
        };
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
