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
    SL_INFO(log(),
            "fetch called Candidate {} chunk {}",
            core.candidate_hash,
            chunk_index);
    std::unique_lock lock{mutex_};
    if (active_.find(core.candidate_hash) != active_.end()) {
      SL_INFO(log(),
              "fetch Candidate {}, chunk {} is already being fetched",
              core.candidate_hash,
              chunk_index);
      return;
    }
    if (av_store_->hasChunk(core.candidate_hash, chunk_index)) {
      SL_INFO(log(),
              "fetch Candidate {}, chunk {} is already available",
              core.candidate_hash,
              chunk_index);
      return;
    }

    SL_INFO(log(),
            "fetch Candidate {}, starting fetch of chunk {}",
            core.candidate_hash,
            chunk_index);

    Active active;
    active.chunk_index = chunk_index;
    active.relay_parent = core.candidate_descriptor.relay_parent;
    active.erasure_encoding_root =
        core.candidate_descriptor.erasure_encoding_root;

    SL_INFO(log(),
            "fetch Candidate {}, setting up active fetch for chunk {}, "
            "erasure_root={}",
            core.candidate_hash,
            chunk_index,
            active.erasure_encoding_root);

    for (auto &validator_index :
         session.validator_groups[core.group_responsible]) {
      active.validators.emplace_back(session.discovery_keys[validator_index]);
      SL_INFO(
          log(),
          "fetch Candidate {}, added validator #{} to fetch list for chunk {}",
          core.candidate_hash,
          validator_index,
          chunk_index);
    }

    SL_INFO(log(),
            "fetch Candidate {}, prepared {} validators for fetching chunk {}",
            core.candidate_hash,
            active.validators.size(),
            chunk_index);

    active_[core.candidate_hash] = std::move(active);
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
      auto req_chunk_version = pm_->getReqChunkVersion(peer_id).value_or(
          network::ReqChunkVersion::V2);

      SL_INFO(log(),
              "Pending requests before increment: {} candidate={} index={}",
              active.pending_requests.size(),
              candidate_hash,
              active.chunk_index);

      const auto request_start = std::chrono::steady_clock::now();
      switch (req_chunk_version) {
        case network::ReqChunkVersion::V2:
          SL_INFO(log(),
                  "Sent request of chunk {} of candidate {} to peer {}",
                  active.chunk_index,
                  candidate_hash,
                  peer_id);
          active.pending_requests.insert(peer_id);
          SL_INFO(log(),
                  "Pending requests after increment: {} candidate={} index={}",
                  active.pending_requests.size(),
                  candidate_hash,
                  active.chunk_index);
          router_->getFetchChunkProtocol()->doRequest(
              peer_id,
              {candidate_hash, active.chunk_index},
              [=, chunk_index{active.chunk_index}, weak{weak_from_this()}](
                  outcome::result<network::FetchChunkResponse> r) {
                if (auto self = weak.lock()) {
                  const auto request_duration_ms =
                      std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now() - request_start)
                          .count();
                  if (r.has_value()) {
                    SL_INFO(log(),
                            "Result of request chunk {} of candidate {} to "
                            "peer {}: success, duration {} ms",
                            chunk_index,
                            candidate_hash,
                            peer_id,
                            request_duration_ms);
                  } else {
                    SL_INFO(log(),
                            "Result of request chunk {} of candidate {} to "
                            "peer {}: {}, duration {} ms",
                            chunk_index,
                            candidate_hash,
                            peer_id,
                            r.error(),
                            request_duration_ms);
                  }

                  self->fetch(candidate_hash, std::move(r), peer_id);
                }
              });
          break;

        case network::ReqChunkVersion::V1_obsolete:
          SL_INFO(log(),
                  "Obsolete request of chunk {} of candidate {} to peer {}",
                  active.chunk_index,
                  candidate_hash,
                  peer_id);
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
                    self->fetch(candidate_hash, std::move(response), peer_id);
                  } else {
                    self->fetch(candidate_hash, r.as_failure(), peer_id);
                  }
                }
              });
          break;

        default:
          SL_ERROR(log(), "Unknown request chunk version for peer {}", peer_id);
          break;
      }
      return;
    }

    // If we get here, we've tried all validators and have no more to try
    // Only remove from active_ if there are no pending requests
    if (active.pending_requests.empty()) {
      SL_INFO(log(),
              "candidate={} chunk={} not found, no more validators to try",
              candidate_hash,
              active.chunk_index);
      active_.erase(it);
    } else {
      SL_INFO(log(),
              "candidate={} chunk={} no more validators to try, but {} pending "
              "requests",
              candidate_hash,
              active.chunk_index,
              active.pending_requests.size());
    }
  }

  void FetchImpl::fetch(const CandidateHash &candidate_hash,
                        outcome::result<network::FetchChunkResponse> _chunk,
                        const libp2p::peer::PeerId &peer_id) {
    SL_INFO(log(), "fetch chunk for {}", candidate_hash);
    std::unique_lock lock{mutex_};
    auto it = active_.find(candidate_hash);
    if (it == active_.end()) {
      SL_INFO(log(), "fetch candidate not found in active {}", candidate_hash);
      return;
    }
    SL_INFO(log(), "fetch candidate found in active {}", candidate_hash);
    auto &active = it->second;

    // Decrement pending requests counter
    SL_INFO(log(),
            "Pending requests before decrement: {} candidate={} index={}",
            active.pending_requests.size(),
            candidate_hash,
            active.chunk_index);
    if (not active.pending_requests.empty()) {
      active.pending_requests.erase(peer_id);
      SL_INFO(log(),
              "Pending requests after decrement: {} candidate={} index={}",
              active.pending_requests.size(),
              candidate_hash,
              active.chunk_index);
    }

    if (_chunk) {
      SL_INFO(log(), "fetch chunk is okay for candidate {}", candidate_hash);
      if (auto chunk2 = boost::get<network::Chunk>(&_chunk.value())) {
        SL_INFO(log(),
                "fetch got chunk for candidate={} index={}",
                candidate_hash,
                active.chunk_index);

        network::ErasureChunk chunk{
            .chunk = std::move(chunk2->data),
            .index = active.chunk_index,
            .proof = std::move(chunk2->proof),
        };
        if (checkTrieProof(chunk, active.erasure_encoding_root)) {
          SL_INFO(log(),
                  "fetch trie proof check passed for candidate={} index={}",
                  candidate_hash,
                  active.chunk_index);
          av_store_->putChunk(
              active.relay_parent, candidate_hash, std::move(chunk));
          SL_VERBOSE(log(),
                     "candidate={} chunk={} fetched",
                     candidate_hash,
                     active.chunk_index);
          active_.erase(it);
          return;
        } else {
          SL_INFO(log(),
                  "fetch trie proof check failed for candidate={} index={}",
                  candidate_hash,
                  active.chunk_index);
        }
      } else {
        SL_INFO(log(),
                "fetch received non-chunk response for candidate={}",
                candidate_hash);
      }
    } else {
      SL_INFO(log(),
              "chunk is not okay for candidate={}, error: {}",
              candidate_hash,
              _chunk.error());
    }

    // If we have no more validators to try and no pending requests, remove from
    // active_
    if (active.validators.empty() && active.pending_requests.empty()) {
      SL_INFO(log(),
              "candidate={} chunk={} failed, no more validators to try and no "
              "pending requests",
              candidate_hash,
              active.chunk_index);
      active_.erase(it);
      return;
    }

    lock.unlock();
    SL_INFO(log(),
            "retrying fetch for candidate={} index={}",
            candidate_hash,
            active.chunk_index);
    fetch(candidate_hash);
  }
}  // namespace kagome::parachain
