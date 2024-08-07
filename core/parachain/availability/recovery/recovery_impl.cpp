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
    Lock lock{mutex_};
    active_.erase(candidate);
    cached_.erase(candidate);
  }

  void RecoveryImpl::recover(const HashedCandidateReceipt &hashed_receipt,
                             SessionIndex session_index,
                             std::optional<GroupIndex> backing_group,
                             std::optional<CoreIndex> core_index,
                             Cb cb) {
    Lock lock{mutex_};
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
    auto chunk_mapping_is_enabled =
        availability_chunk_mapping_is_enabled(_node_features.value());

    Active active;
    active.erasure_encoding_root = receipt.descriptor.erasure_encoding_root;
    active.chunks_total = session->validators.size();
    active.chunks_required = _min.value();
    active.cb.emplace_back(std::move(cb));
    active.validators = session->discovery_keys;
    active.val2chunk = [chunk_mapping_is_enabled,
                        n_validators{session->validators.size()},
                        core_start_pos{core_index.value() * _min.value()}]  //
        (ValidatorIndex validator_index) -> ChunkIndex {
      if (chunk_mapping_is_enabled) {
        return (core_start_pos + validator_index) % n_validators;
      }
      return validator_index;
    };

    std::vector<ValidatorIndex> validators_of_group;

    if (backing_group) {
      active.validators_of_group = session->validator_groups.at(*backing_group);
    }
    active_.emplace(candidate_hash, std::move(active));

    lock.unlock();

    full_from_bakers_recovery_prepare(candidate_hash);
  }

  void RecoveryImpl::full_from_bakers_recovery_prepare(
      const CandidateHash &candidate_hash) {
    Lock lock{mutex_};
    auto it = active_.find(candidate_hash);
    if (it == active_.end()) {
      return;
    }
    auto &active = it->second;

    // Fill request order by validators of group
    active.order = std::move(active.validators_of_group);
    std::shuffle(active.order.begin(), active.order.end(), random_);

    // Is it possible to full recover from bakers
    auto is_possible_to_recovery_from_bakers = not active.order.empty();

    if (is_possible_to_recovery_from_bakers) {
      lock.unlock();
      full_from_bakers_recovery(candidate_hash);
    } else {
      systematic_chunks_recovery_prepare(candidate_hash);
    }
  }

  void RecoveryImpl::full_from_bakers_recovery(
      const CandidateHash &candidate_hash) {
    Lock lock{mutex_};
    auto it = active_.find(candidate_hash);
    if (it == active_.end()) {
      return;
    }
    auto &active = it->second;

    // Send requests
    while (not active.order.empty()) {
      auto peer = query_audi_->get(active.validators[active.order.back()]);
      active.order.pop_back();
      if (peer) {
        send_fetch_available_data_request(
            peer->id, candidate_hash, &RecoveryImpl::full_from_bakers_recovery);
        return;
      }
    }

    // No known peer anymore to do full recovery
    lock.unlock();

    systematic_chunks_recovery_prepare(candidate_hash);
  }

  void RecoveryImpl::systematic_chunks_recovery_prepare(
      const CandidateHash &candidate_hash) {
    Lock lock{mutex_};
    auto it = active_.find(candidate_hash);
    if (it == active_.end()) {
      return;
    }
    auto &active = it->second;

    // Refill request order basing chunks
    active.chunks = av_store_->getChunks(candidate_hash);
    for (size_t validator_index = 0; validator_index < active.chunks_total;
         ++validator_index) {
      auto chunk_index = active.val2chunk(validator_index);

      // Filter non systematic chunks
      if (chunk_index > active.chunks_required) {
        continue;
      }

      // Filter existing
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
    active.queried.clear();

    size_t systematic_chunk_count = [&] {
      std::set<ChunkIndex> sci;
      for (auto &chunk : active.chunks) {
        if (chunk.index <= active.chunks_required) {
          sci.emplace(chunk.index);
        }
      }
      return sci.size();
    }();

    // Is it possible to collect all systematic chunks?
    auto is_possible_to_collect_systematic_chunks =
        systematic_chunk_count + active.chunks_active + active.order.size()
        >= active.chunks_required;

    if (is_possible_to_collect_systematic_chunks) {
      lock.unlock();
      systematic_chunks_recovery(candidate_hash);
    } else {
      regular_chunks_recovery_prepare(candidate_hash);
    }
  }

  void RecoveryImpl::systematic_chunks_recovery(
      const CandidateHash &candidate_hash) {
    Lock lock{mutex_};
    auto it = active_.find(candidate_hash);
    if (it == active_.end()) {
      return;
    }
    auto &active = it->second;

    size_t systematic_chunk_count = [&] {
      std::set<ChunkIndex> sci;
      for (auto &chunk : active.chunks) {
        if (chunk.index <= active.chunks_required) {
          sci.emplace(chunk.index);
        }
      }
      return sci.size();
    }();

    // All systematic chunks are collected
    if (systematic_chunk_count >= active.chunks_required) {
      auto _data =
          fromSystematicChunks(active.validators.size(), active.chunks);
      if (_data) {
        if (auto r = check(active, _data.value()); not r) {
          _data = r.error();
        }
      }
      return done(lock, it, _data, Strategy::SystematicChunks);
    }

    // Send requests
    auto max = std::min(kParallelRequests,
                        active.chunks_required - systematic_chunk_count);
    while (not active.order.empty() and active.chunks_active < max) {
      auto validator_index = active.order.back();
      active.order.pop_back();
      auto peer = query_audi_->get(active.validators[validator_index]);
      if (peer) {
        ++active.chunks_active;
        active.queried.emplace(validator_index);
        send_fetch_chunk_request(
            peer->id,
            candidate_hash,
            active.val2chunk(validator_index),  // chunk_index
            &RecoveryImpl::systematic_chunks_recovery);
      }
    }

    lock.unlock();

    // No active request anymore for systematic chunks recovery
    if (active.chunks_active == 0) {
      RecoveryImpl::regular_chunks_recovery_prepare(candidate_hash);
    }
  }

  void RecoveryImpl::regular_chunks_recovery_prepare(
      const CandidateHash &candidate_hash) {
    Lock lock{mutex_};
    auto it = active_.find(candidate_hash);
    if (it == active_.end()) {
      return;
    }
    auto &active = it->second;

    // Refill request order by remaining validators
    active.chunks = av_store_->getChunks(candidate_hash);
    for (size_t validator_index = 0; validator_index < active.chunks_total;
         ++validator_index) {
      // Filter queried
      if (active.queried.contains(validator_index)) {
        continue;
      }

      // Filter existing
      if (std::find_if(
              active.chunks.begin(),
              active.chunks.end(),
              [chunk_index{active.val2chunk(validator_index)}](
                  network::ErasureChunk &c) { return c.index == chunk_index; })
          != active.chunks.end()) {
        continue;
      }

      active.order.emplace_back(validator_index);
    }
    std::shuffle(active.order.begin(), active.order.end(), random_);

    // Is it possible to collect enough chunks for recovery?
    auto is_possible_to_collect_systematic_chunks =
        active.chunks.size() + active.chunks_active + active.order.size()
        >= active.chunks_required;

    if (is_possible_to_collect_systematic_chunks) {
      lock.unlock();
      regular_chunks_recovery(candidate_hash);
    } else {
      return done(lock, it, std::nullopt, Strategy::RegularChunks);
    }
  }

  void RecoveryImpl::regular_chunks_recovery(
      const CandidateHash &candidate_hash) {
    Lock lock{mutex_};
    auto it = active_.find(candidate_hash);
    if (it == active_.end()) {
      return;
    }
    auto &active = it->second;

    // The number of chunks is enough for regular chunk recovery
    if (active.chunks.size() >= active.chunks_required) {
      auto _data = fromChunks(active.chunks_total, active.chunks);
      if (_data) {
        if (auto r = check(active, _data.value()); not r) {
          _data = r.error();
        }
      }
      return done(lock, it, _data, Strategy::RegularChunks);
    }

    // Send requests
    auto max = std::min(kParallelRequests,
                        active.chunks_required - active.chunks.size());
    while (not active.order.empty() and active.chunks_active < max) {
      auto validator_index = active.order.back();
      active.order.pop_back();
      auto peer = query_audi_->get(active.validators[validator_index]);
      if (peer.has_value()) {
        ++active.chunks_active;
        active.queried.emplace(validator_index);
        send_fetch_chunk_request(
            peer->id,
            candidate_hash,
            active.val2chunk(validator_index),  // chunk_index
            &RecoveryImpl::regular_chunks_recovery);
      }
    }

    // No active request anymore for regular chunks recovery
    if (active.chunks_active == 0) {
      done(lock, it, std::nullopt, Strategy::RegularChunks);
    }
  }

  // Fetch available data protocol communication
  void RecoveryImpl::send_fetch_available_data_request(
      const libp2p::PeerId &peer_id,
      const CandidateHash &candidate_hash,
      void (RecoveryImpl::*cb)(const CandidateHash &)) {
    router_->getFetchAvailableDataProtocol()->doRequest(
        peer_id,
        candidate_hash,
        [=, weak{weak_from_this()}](
            outcome::result<network::FetchAvailableDataResponse> response_res) {
          if (auto self = weak.lock()) {
            self->handle_fetch_available_data_response(
                candidate_hash, std::move(response_res), cb);
          }
        });
  }

  void RecoveryImpl::handle_fetch_available_data_response(
      const CandidateHash &candidate_hash,
      outcome::result<network::FetchAvailableDataResponse> response_res,
      void (RecoveryImpl::*next_iteration)(const CandidateHash &)) {
    Lock lock{mutex_};
    auto it = active_.find(candidate_hash);
    if (it == active_.end()) {
      return;
    }

    auto &active = it->second;

    if (response_res) {
      if (auto data = boost::get<AvailableData>(&response_res.value())) {
        if (check(active, *data)) {
          return done(lock, it, std::move(*data), Strategy::FullFromBackers);
        }
      }
    }

    lock.unlock();

    (this->*next_iteration)(candidate_hash);
  }

  void RecoveryImpl::send_fetch_chunk_request(
      const libp2p::PeerId &peer_id,
      const CandidateHash &candidate_hash,
      ChunkIndex chunk_index,
      void (RecoveryImpl::*cb)(const CandidateHash &)) {
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
        SL_DEBUG(logger_,
                 "Sent request of chunk {} of candidate {} to peer {}",
                 chunk_index,
                 candidate_hash,
                 peer_id);
        router_->getFetchChunkProtocol()->doRequest(
            peer_id,
            {candidate_hash, chunk_index},
            [=, this, weak{weak_from_this()}](
                outcome::result<network::FetchChunkResponse> response_res) {
              if (auto self = weak.lock()) {
                if (response_res.has_value()) {
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
                           response_res.error());
                }

                self->handle_fetch_chunk_response(
                    candidate_hash, std::move(response_res), cb);
              }
            });
      } break;
      case network::ReqChunkVersion::V1_obsolete: {
        router_->getFetchChunkProtocolObsolete()->doRequest(
            peer_id,
            {candidate_hash, chunk_index},
            [=, weak{weak_from_this()}](
                outcome::result<network::FetchChunkResponseObsolete>
                    response_res) {
              if (auto self = weak.lock()) {
                if (response_res.has_value()) {
                  auto response = visit_in_place(
                      response_res.value(),
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
                  self->handle_fetch_chunk_response(
                      candidate_hash, std::move(response), cb);
                } else {
                  self->handle_fetch_chunk_response(
                      candidate_hash, response_res.as_failure(), cb);
                }
              }
            });
      } break;
      default:
        UNREACHABLE;
    }
  }

  void RecoveryImpl::handle_fetch_chunk_response(
      const CandidateHash &candidate_hash,
      outcome::result<network::FetchChunkResponse> response_res,
      void (RecoveryImpl::*next_iteration)(const CandidateHash &)) {
    Lock lock{mutex_};
    auto it = active_.find(candidate_hash);
    if (it == active_.end()) {
      return;
    }
    auto &active = it->second;

    --active.chunks_active;

    if (response_res.has_value()) {
      if (auto chunk = boost::get<network::Chunk>(&response_res.value())) {
        network::ErasureChunk erasure_chunk{
            std::move(chunk->data),
            chunk->chunk_index,
            std::move(chunk->proof),
        };
        if (checkTrieProof(erasure_chunk, active.erasure_encoding_root)) {
          active.chunks.emplace_back(std::move(erasure_chunk));
        }
      }
    }

    lock.unlock();

    (this->*next_iteration)(candidate_hash);
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
