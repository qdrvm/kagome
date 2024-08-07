/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/availability/recovery/recovery_impl.hpp"

#include "application/chain_spec.hpp"
#include "network/impl/protocols/protocol_fetch_available_data.hpp"
#include "network/impl/protocols/protocol_fetch_chunk.hpp"
#include "network/impl/protocols/protocol_fetch_chunk_obsolete.hpp"
#include "network/peer_manager.hpp"
#include "parachain/availability/availability_chunk_index.hpp"
#include "parachain/availability/chunks.hpp"
#include "parachain/availability/proof.hpp"
#include "parachain/availability/store/store.hpp"

namespace {
  constexpr auto fullRecoveriesStartedMetricName =
      "kagome_parachain_availability_recovery_recoveries_started";
  constexpr auto fullRecoveriesFinishedMetricName =
      "kagome_parachain_availability_recovery_recoveries_finished";

  const std::array<std::string, 4> strategy_types = {
      "full_from_backers", "systematic_chunks", "regular_chunks", "all"};

  const std::array<std::string, 3> results = {"success", "failure", "invalid"};

#define incFullRecoveriesFinished(strategy, result)                         \
  do {                                                                      \
    BOOST_ASSERT_MSG(                                                       \
        std::find(strategy_types.begin(), strategy_types.end(), strategy)   \
            != strategy_types.end(),                                        \
        "Unknown strategy type");                                           \
    BOOST_ASSERT_MSG(                                                       \
        std::find(results.begin(), results.end(), result) != results.end(), \
        "Unknown result type");                                             \
    full_recoveries_finished_.at(strategy).at(result)->inc();               \
  } while (false)

}  // namespace

namespace kagome::parachain {
  constexpr size_t kParallelRequests = 50;

  RecoveryImpl::RecoveryImpl(
      std::shared_ptr<application::ChainSpec> chain_spec,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<runtime::ParachainHost> parachain_api,
      std::shared_ptr<AvailabilityStore> av_store,
      std::shared_ptr<authority_discovery::Query> query_audi,
      std::shared_ptr<network::Router> router,
      std::shared_ptr<network::PeerManager> pm)
      : logger_{log::createLogger("Recovery", "parachain")},
        hasher_{std::move(hasher)},
        block_tree_{std::move(block_tree)},
        parachain_api_{std::move(parachain_api)},
        av_store_{std::move(av_store)},
        query_audi_{std::move(query_audi)},
        router_{std::move(router)},
        pm_{std::move(pm)} {
    // Register metrics
    metrics_registry_->registerCounterFamily(
        fullRecoveriesStartedMetricName, "Total number of started recoveries");
    full_recoveries_started_ = metrics_registry_->registerCounterMetric(
        fullRecoveriesStartedMetricName);
    metrics_registry_->registerCounterFamily(
        fullRecoveriesFinishedMetricName,
        "Total number of recoveries that finished");

    for (auto &strategy : strategy_types) {
      auto &metrics_for_strategy = full_recoveries_finished_[strategy];
      for (auto &result : results) {
        auto &metrics_for_result = metrics_for_strategy[result];
        metrics_for_result = metrics_registry_->registerCounterMetric(
            fullRecoveriesFinishedMetricName,
            {{"result", std::string(result)},
             {"strategy_type", std::string(strategy)},
             {"chain", chain_spec->chainType()}});
      }
    }

    BOOST_ASSERT(pm_);
  }

  void RecoveryImpl::remove(const CandidateHash &candidate) {
    std::unique_lock lock{mutex_};
    active_.erase(candidate);
    cached_.erase(candidate);
  }

  void RecoveryImpl::recover(const HashedCandidateReceipt &hashed_receipt,
                             SessionIndex session_index,
                             std::optional<GroupIndex> backing_group,
                             CoreIndex core_index,
                             Cb cb) {
    std::unique_lock lock{mutex_};
    const auto &receipt = hashed_receipt.get();
    const auto &candidate_hash = hashed_receipt.getHash();
    if (auto it = cached_.find(candidate_hash); it != cached_.end()) {
      auto r = it->second;
      lock.unlock();
      cb(std::move(r));
      return;
    }
    if (auto it = active_.find(candidate_hash); it != active_.end()) {
      it->second.cb.emplace_back(std::move(cb));
      return;
    }
    if (auto data = av_store_->getPovAndData(candidate_hash)) {
      cached_.emplace(candidate_hash, *data);
      lock.unlock();
      cb(std::move(*data));
      return;
    }
    auto block = block_tree_->bestBlock();
    auto _session = parachain_api_->session_info(block.hash, session_index);
    if (not _session) {
      lock.unlock();
      cb(_session.error());
      return;
    }
    auto &session = _session.value();
    auto _min = minChunks(session->validators.size());
    if (not _min) {
      lock.unlock();
      cb(_min.error());
      return;
    }
    auto _node_features =
        parachain_api_->node_features(block.hash, session_index);
    if (_node_features.has_error()) {
      lock.unlock();
      cb(_node_features.error());
      return;
    }

    Active active;
    active.erasure_encoding_root = receipt.descriptor.erasure_encoding_root;
    active.chunks_total = session->validators.size();
    active.chunks_required = _min.value();
    active.cb.emplace_back(std::move(cb));
    active.validators = session->discovery_keys;
    active.node_features = _node_features.value();
    active.core = core_index;
    if (backing_group) {
      active.order = session->validator_groups.at(*backing_group);
      std::shuffle(active.order.begin(), active.order.end(), random_);
    }
    active_.emplace(candidate_hash, std::move(active));
    lock.unlock();
    full_recovery(candidate_hash);
  }

  void RecoveryImpl::full_recovery(const CandidateHash &candidate_hash) {
    std::unique_lock lock{mutex_};
    auto it = active_.find(candidate_hash);
    if (it == active_.end()) {
      return;
    }
    auto &active = it->second;
    while (not active.order.empty()) {
      auto peer = query_audi_->get(active.validators[active.order.back()]);
      active.order.pop_back();
      if (peer) {
        router_->getFetchAvailableDataProtocol()->doRequest(
            peer->id,
            candidate_hash,
            [=, weak{weak_from_this()}](
                outcome::result<network::FetchAvailableDataResponse> r) {
              if (auto self = weak.lock()) {
                self->full_recovery_iteration(candidate_hash, std::move(r));
              }
            });
        return;
      }
    }

    // No known peer anymore to do full recovery
    // Refill request order basing chunks
    active.chunks = av_store_->getChunks(candidate_hash);
    for (size_t validator_index = 0; validator_index < active.chunks_total;
         ++validator_index) {
      // TODO think about replace it by func with once computed context
      auto chunk_res = availability_chunk_index(active.node_features,
                                                active.validators.size(),
                                                active.core,
                                                validator_index);
      if (chunk_res.has_error()) {
        continue;
      }
      auto chunk_index = chunk_res.value();

      if (std::find_if(
              active.chunks.begin(),
              active.chunks.end(),
              [&](network::ErasureChunk &c) { return c.index == chunk_index; })
          != active.chunks.end()) {
        continue;
      }
      active.order.emplace_back(validator_index);
    }
    std::shuffle(active.order.begin(), active.order.end(), random_);
    lock.unlock();
    systematic_recovery(candidate_hash);
  }

  void RecoveryImpl::full_recovery_iteration(
      const CandidateHash &candidate_hash,
      outcome::result<network::FetchAvailableDataResponse> _backed) {
    std::unique_lock lock{mutex_};
    auto it = active_.find(candidate_hash);
    if (it == active_.end()) {
      return;
    }
    auto &active = it->second;
    if (_backed) {
      if (auto data = boost::get<AvailableData>(&_backed.value())) {
        if (check(active, *data)) {
          return done(lock, it, std::move(*data), Strategy::FullFromBackers);
        }
      }
    }
    lock.unlock();
    full_recovery(candidate_hash);
  }

  void RecoveryImpl::systematic_recovery(const CandidateHash &candidate_hash) {
    std::unique_lock lock{mutex_};
    auto it = active_.find(candidate_hash);
    if (it == active_.end()) {
      return;
    }
    auto &active = it->second;

    // if chunk mapping is not enabled, systematic can't be enabled either
    if (!availability_chunk_mapping_is_enabled(active.node_features)) {
      return chunk_recovery(candidate_hash);
    }

    auto st_res = systematic_recovery_threshold(active.validators.size());
    if (st_res.has_error()) {
      return chunk_recovery(candidate_hash);
    }
    auto systematic_threshold = st_res.value();

    size_t systematic_chunk_count = [&] {
      std::set<ChunkIndex> sci;
      for (auto &chunk : active.chunks) {
        if (chunk.index <= systematic_threshold) {
          sci.emplace(chunk.index);
        }
      }
      return sci.size();
    }();

    if (active.chunks.size() >= active.chunks_required) {
      auto _data = fromChunks(active.validators.size(), active.chunks);
      if (_data) {
        if (auto r = check(active, _data.value()); not r) {
          _data = r.error();
        }
      }
      return done(lock, it, _data, Strategy::SystematicChunks);
    }
    if (active.chunks.size() + active.chunks_active + active.order.size()
        < active.chunks_required) {
      return done(lock, it, std::nullopt, Strategy::SystematicChunks);
    }

    auto max = std::min(kParallelRequests,
                        active.chunks_required - active.chunks.size());
    while (not active.order.empty() and active.chunks_active < max) {
      auto validator_index = active.order.back();
      active.order.pop_back();
      auto peer = query_audi_->get(active.validators[validator_index]);
      if (peer) {
        ++active.chunks_active;
        router_->getFetchChunkProtocol()->doRequest(
            peer->id,
            {candidate_hash, validator_index},
            [=, weak{weak_from_this()}](
                outcome::result<network::FetchChunkResponse> r) {
              auto self = weak.lock();
              if (not self) {
                return;
              }
              self->chunk_recovery_iteration(
                  candidate_hash, validator_index, std::move(r));
            });
        return;
      }
    }
    if (active.chunks_active == 0) {
      done(lock, it, std::nullopt, Strategy::SystematicChunks);
    }
  }

  void RecoveryImpl::systematic_recovery_iteration(
      const CandidateHash &candidate_hash,
      ValidatorIndex index,
      outcome::result<network::FetchChunkResponse> _chunk) {
    std::unique_lock lock{mutex_};
    auto it = active_.find(candidate_hash);
    if (it == active_.end()) {
      return;
    }
    auto &active = it->second;
    --active.chunks_active;
    if (_chunk) {
      if (auto chunk2 = boost::get<network::Chunk>(&_chunk.value())) {
        network::ErasureChunk chunk{
            std::move(chunk2->data), index, std::move(chunk2->proof)};
        if (checkTrieProof(chunk, active.erasure_encoding_root)) {
          active.chunks.emplace_back(std::move(chunk));
        }
      }
    }
    lock.unlock();
    chunk_recovery(candidate_hash);
  }

  void RecoveryImpl::chunk_recovery(const CandidateHash &candidate_hash) {
    std::unique_lock lock{mutex_};
    auto it = active_.find(candidate_hash);
    if (it == active_.end()) {
      return;
    }
    auto &active = it->second;
    if (active.chunks.size() >= active.chunks_required) {
      auto _data = fromChunks(active.chunks_total, active.chunks);
      if (_data) {
        if (auto r = check(active, _data.value()); not r) {
          _data = r.error();
        }
      }
      return done(lock, it, _data, Strategy::RegularChunks);
    }
    if (active.chunks.size() + active.chunks_active + active.order.size()
        < active.chunks_required) {
      return done(lock, it, std::nullopt, Strategy::RegularChunks);
    }
    auto max = std::min(kParallelRequests,
                        active.chunks_required - active.chunks.size());
    while (not active.order.empty() and active.chunks_active < max) {
      auto validator_index = active.order.back();
      active.order.pop_back();
      auto peer = query_audi_->get(active.validators[validator_index]);
      if (peer) {
        ++active.chunks_active;

        const auto &peer_id = peer.value().id;
        auto peer_state = [&]() {
          auto res = pm_->getPeerState(peer_id);
          if (!res) {
            SL_TRACE(logger_, "From unknown peer {}", peer_id);
            res = pm_->createDefaultPeerState(peer_id);
          }
          return res;
        }();

        auto req_chunk_version = peer_state->get().req_chunk_version.value_or(
            network::ReqChunkVersion::V1_obsolete);

        switch (req_chunk_version) {
          case network::ReqChunkVersion::V2: {
            // TODO think about replace it by func with once computed context
            auto chunk_res = availability_chunk_index(active.node_features,
                                                      active.validators.size(),
                                                      active.core,
                                                      validator_index);
            if (chunk_res.has_error()) {
              continue;
            }
            auto chunk_index = chunk_res.value();

            SL_DEBUG(logger_,
                     "Sent request of chunk {} of candidate {} to peer {}",
                     chunk_index,
                     candidate_hash,
                     peer_id);
            router_->getFetchChunkProtocol()->doRequest(
                peer->id,
                {candidate_hash, chunk_index},
                [=, this, weak{weak_from_this()}](
                    outcome::result<network::FetchChunkResponse> r) {
                  if (auto self = weak.lock()) {
                    if (r.has_value()) {
                      SL_DEBUG(logger_,
                               "Result of request chunk {} of candidate {} to "
                               "peer {}: success",
                               chunk_index,
                               candidate_hash,
                               peer_id);
                    } else {
                      SL_DEBUG(logger_,
                               "Result of request chunk {} of candidate {} to "
                               "peer {}: {}",
                               chunk_index,
                               candidate_hash,
                               peer_id,
                               r.error());
                    }

                    self->chunk_recovery_iteration(
                        candidate_hash, chunk_index, std::move(r));
                  }
                });
          } break;
          case network::ReqChunkVersion::V1_obsolete: {
            ChunkIndex chunk_index = validator_index;
            router_->getFetchChunkProtocolObsolete()->doRequest(
                peer->id,
                {candidate_hash, chunk_index},
                [=, weak{weak_from_this()}](
                    outcome::result<network::FetchChunkResponseObsolete> r) {
                  if (auto self = weak.lock()) {
                    if (r.has_value()) {
                      auto response = visit_in_place(
                          r.value(),
                          [](const network::Empty &empty)
                              -> network::FetchChunkResponse { return empty; },
                          [&](const network::ChunkObsolete &chunk_obsolete)
                              -> network::FetchChunkResponse {
                            return network::Chunk{
                                .data = std::move(chunk_obsolete.data),
                                .chunk_index = chunk_index,
                                .proof = std::move(chunk_obsolete.proof),
                            };
                          });
                      self->chunk_recovery_iteration(
                          candidate_hash, chunk_index, std::move(response));
                    } else {
                      self->chunk_recovery_iteration(
                          candidate_hash, chunk_index, r.as_failure());
                    }
                  }
                });
          } break;
          default:
            UNREACHABLE;
        }
        return;
      }
    }
    if (active.chunks_active == 0) {
      done(lock, it, std::nullopt, Strategy::RegularChunks);
    }
  }

  void RecoveryImpl::chunk_recovery_iteration(
      const CandidateHash &candidate_hash,
      ChunkIndex chunk_index,
      outcome::result<network::FetchChunkResponse> _chunk) {
    std::unique_lock lock{mutex_};
    auto it = active_.find(candidate_hash);
    if (it == active_.end()) {
      return;
    }
    auto &active = it->second;
    --active.chunks_active;
    if (_chunk) {
      if (auto chunk2 = boost::get<network::Chunk>(&_chunk.value())) {
        network::ErasureChunk chunk{
            std::move(chunk2->data), chunk_index, std::move(chunk2->proof)};
        if (checkTrieProof(chunk, active.erasure_encoding_root)) {
          active.chunks.emplace_back(std::move(chunk));
        }
      }
    }
    lock.unlock();
    chunk_recovery(candidate_hash);
  }

  outcome::result<void> RecoveryImpl::check(const Active &active,
                                            const AvailableData &data) {
    OUTCOME_TRY(chunks, toChunks(active.chunks_total, data));
    auto root = makeTrieProof(chunks);
    if (root != active.erasure_encoding_root) {
      return ErasureCodingRootError::MISMATCH;
    }
    return outcome::success();
  }

  void RecoveryImpl::done(
      Lock &lock,
      ActiveMap::iterator it,
      const std::optional<outcome::result<AvailableData>> &result_op,
      Strategy strategy) {
    auto status = "failure";

    if (result_op.has_value()) {
      auto &result = result_op.value();
      cached_.emplace(it->first, result);

      if (result.has_error()) {
        status = "invalid";
      } else {
        status = "success";
      }
    }

    switch (strategy) {
      case Strategy::FullFromBackers:
        incFullRecoveriesFinished("full_from_backers", status);
        break;
      case Strategy::SystematicChunks:
        incFullRecoveriesFinished("systematic_chunks", status);
        break;
      case Strategy::RegularChunks:
        incFullRecoveriesFinished("regular_chunks", status);
        break;
    }
    incFullRecoveriesFinished("all", status);

    auto node = active_.extract(it);
    lock.unlock();
    for (auto &cb : node.mapped().cb) {
      cb(result_op);
    }
  }
}  // namespace kagome::parachain
