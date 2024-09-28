/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/availability/recovery/recovery_impl.hpp"

#include "application/chain_spec.hpp"
#include "authority_discovery/query/query.hpp"
#include "blockchain/block_tree.hpp"
#include "log/formatters/optional.hpp"
#include "network/impl/protocols/protocol_fetch_available_data.hpp"
#include "network/impl/protocols/protocol_fetch_chunk.hpp"
#include "network/impl/protocols/protocol_fetch_chunk_obsolete.hpp"
#include "network/peer_manager.hpp"
#include "network/router.hpp"
#include "parachain/availability/availability_chunk_index.hpp"
#include "parachain/availability/chunks.hpp"
#include "parachain/availability/proof.hpp"
#include "parachain/availability/store/store.hpp"
#include "runtime/runtime_api/parachain_host.hpp"

namespace {
  constexpr auto fullRecoveriesStartedMetricName =
      "kagome_parachain_availability_recovery_recoveries_started";
  constexpr auto fullRecoveriesFinishedMetricName =
      "kagome_parachain_availability_recovery_recoveries_finished";

  const std::array<std::string, 4> strategy_types = {
      "full_from_backers", "systematic_chunks", "regular_chunks", "all"};

  const std::array<std::string, 3> results = {"success", "failure", "invalid"};

  // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define incFullRecoveriesFinished(strategy, result)                          \
  [&] {                                                                      \
    BOOST_ASSERT_MSG(                                                        \
        std::ranges::find(strategy_types, strategy) != strategy_types.end(), \
        "Unknown strategy type");                                            \
    BOOST_ASSERT_MSG(std::ranges::find(results, result) != results.end(),    \
                     "Unknown result type");                                 \
    full_recoveries_finished_.at(strategy).at(result)->inc();                \
  }()

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

    BOOST_ASSERT(chain_spec != nullptr);
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

    ValidatorIndex start_pos = 0;
    if (core_index.has_value()) {
      if (availability_chunk_mapping_is_enabled(_node_features.value())) {
        start_pos = core_index.value() * _min.value();
      }
    }

    Active active;
    active.erasure_encoding_root = receipt.descriptor.erasure_encoding_root;
    active.chunks_total = session->validators.size();
    active.chunks_required = _min.value();
    active.cb.emplace_back(std::move(cb));
    active.discovery_keys = session->discovery_keys;
    active.val2chunk = [n_validators{active.chunks_total}, start_pos](
                           ValidatorIndex validator_index) -> ChunkIndex {
      return (start_pos + validator_index) % n_validators;
    };

    if (backing_group.has_value()) {
      const auto group = backing_group.value();
      BOOST_ASSERT(group < session->validator_groups.size());
      active.validators_of_group = session->validator_groups[group];
    }

    SL_TRACE(logger_,
             "Candidate {}. "
             "Start recovery. "
             "Total chunks: {}, required for EC: {}, "
             "discovery keys: {}, "
             "baking group: {}, "
             "validators of group: [{}], "
             "start_pos for mapping: {}",
             candidate_hash,
             active.chunks_total,
             active.chunks_required,
             active.discovery_keys.size(),
             backing_group,
             fmt::join(active.validators_of_group, ", "),
             start_pos);

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

    SL_TRACE(logger_,
             "Candidate {}. "
             "Recovery from bakers preparation. "
             "{} validators of group",
             candidate_hash,
             active.order.size());

    // Is it possible to full recover from bakers
    auto is_possible_to_recovery_from_bakers = not active.order.empty();

    lock.unlock();

    if (not is_possible_to_recovery_from_bakers) {
      SL_TRACE(logger_,
               "Candidate {}. "
               "Impossible to recover from bakers. "
               "No available validators of group. "
               "Trying to do systematic chunks recovery",
               candidate_hash);
      systematic_chunks_recovery_prepare(candidate_hash);
      return;
    }

    full_from_bakers_recovery(candidate_hash);
  }

  void RecoveryImpl::full_from_bakers_recovery(
      const CandidateHash &candidate_hash) {
    Lock lock{mutex_};

    auto it = active_.find(candidate_hash);
    if (it == active_.end()) {
      return;
    }
    auto &active = it->second;

    // Is it possible to full recover from bakers
    auto is_possible_to_recovery_from_bakers = not active.order.empty();

    if (is_possible_to_recovery_from_bakers) {
      SL_TRACE(logger_,
               "Candidate {}. "
               "Remain {} validators to recover from bakers. "
               "Trying to ask next one",
               candidate_hash,
               active.order.size());

      // Send requests
      while (not active.order.empty()) {
        auto validator_index = active.order.back();
        auto peer = query_audi_->get(active.discovery_keys[validator_index]);
        active.order.pop_back();
        if (peer.has_value()) {
          SL_TRACE(logger_,
                   "Candidate {}. "
                   "Asking validator #{} aka peer {}",
                   candidate_hash,
                   validator_index,
                   peer->id);
          send_fetch_available_data_request(
              peer->id,
              candidate_hash,
              &RecoveryImpl::full_from_bakers_recovery);
          return;
        }
        SL_TRACE(logger_,
                 "Candidate {}. "
                 "PeerId of validator #{} is not discovered. Skipping... ",
                 candidate_hash,
                 validator_index);
      }
    }
    lock.unlock();

    SL_TRACE(logger_,
             "Candidate {}. "
             "Impossible to recover from bakers. "
             "No available validators from group anymore. "
             "Trying to do systematic chunks recovery",
             candidate_hash);

    // No known peer anymore to do full recovery
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
    SL_TRACE(logger_,
             "Candidate {}. "
             "Systematic recovery preparation. "
             "Already collected {} any chunks",
             candidate_hash,
             active.chunks.size());

    for (size_t validator_index = 0; validator_index < active.chunks_total;
         ++validator_index) {
      auto chunk_index = active.val2chunk(validator_index);

      // Filter non systematic chunks
      if (chunk_index >= active.chunks_required) {
        SL_TRACE(logger_,
                 "Candidate {}. "
                 "Systematic recovery preparation. "
                 "Validator #{} has ignored as non-holder of systematic chunk",
                 candidate_hash,
                 validator_index);
        continue;
      }

      // Filter existing
      if (std::ranges::find_if(
              active.chunks,
              [&](network::ErasureChunk &c) { return c.index == chunk_index; })
          != active.chunks.end()) {
        SL_TRACE(logger_,
                 "Candidate {}. "
                 "Systematic recovery preparation. "
                 "Validator #{} has ignored because such chunk already exists",
                 candidate_hash,
                 validator_index);
        continue;
      }
      active.order.emplace_back(validator_index);
    }
    std::shuffle(active.order.begin(), active.order.end(), random_);
    active.queried.clear();
    active.chunks_active = 0;

    SL_TRACE(logger_,
             "Candidate {}. "
             "Systematic recovery preparation. "
             "{} validators in order for asking",
             candidate_hash,
             active.order.size());

    size_t systematic_chunk_count = [&] {
      std::set<ChunkIndex> sci;
      for (auto &chunk : active.chunks) {
        if (chunk.index < active.chunks_required) {
          sci.emplace(chunk.index);
        }
      }
      return sci.size();
    }();

    SL_TRACE(logger_,
             "Candidate {}. "
             "Systematic recovery preparation. "
             "Already collected {} systematic chunks",
             candidate_hash,
             systematic_chunk_count);

    // Is it possible to collect all systematic chunks?
    bool is_possible_to_collect_systematic_chunks =
        systematic_chunk_count + active.chunks_active + active.order.size()
        >= active.chunks_required;

    lock.unlock();

    if (not is_possible_to_collect_systematic_chunks) {
      SL_TRACE(logger_,
               "Candidate {}. "
               "Impossible to do systematic chunk recovery "
               "({} chunks + {} in order < {} required). "
               "Trying to do regular chunks recovery",
               candidate_hash,
               systematic_chunk_count,
               active.order.size(),
               active.chunks_required);
      regular_chunks_recovery_prepare(candidate_hash);
      return;
    }

    systematic_chunks_recovery(candidate_hash);
  }

  void RecoveryImpl::systematic_chunks_recovery(
      const CandidateHash &candidate_hash) {
    Lock lock{mutex_};

    auto it = active_.find(candidate_hash);
    if (it == active_.end()) {
      return;
    }
    auto &active = it->second;

    if (active.systematic_chunk_failed) {
      lock.unlock();
      return regular_chunks_recovery(candidate_hash);
    }

    size_t systematic_chunk_count = [&] {
      std::set<ChunkIndex> sci;
      for (auto &chunk : active.chunks) {
        if (chunk.index < active.chunks_required) {
          sci.emplace(chunk.index);
        }
      }
      return sci.size();
    }();

    SL_TRACE(logger_,
             "Candidate {}. "
             "Systematic recovery progress. "
             "Collected {} systematic chunks",
             candidate_hash,
             systematic_chunk_count);

    // All systematic chunks are collected
    if (systematic_chunk_count >= active.chunks_required) {
      SL_TRACE(logger_,
               "Candidate {}. "
               "Systematic recovery progress. "
               "Collected all required chunks ({} of {})",
               candidate_hash,
               systematic_chunk_count,
               active.chunks_required);

      auto data_res = fromSystematicChunks(active.chunks_total, active.chunks);
      [[unlikely]] if (data_res.has_error()) {
        active.systematic_chunk_failed = true;
        SL_DEBUG(logger_,
                 "Systematic data recovery error "
                 "(candidate={}, erasure_root={}): {}",
                 candidate_hash,
                 active.erasure_encoding_root,
                 data_res.error());
        incFullRecoveriesFinished("systematic_chunks", "invalid");
      } else {
        auto &data = data_res.value();
        auto res = check(active, data);
        [[unlikely]] if (res.has_error()) {
          active.systematic_chunk_failed = true;
          SL_DEBUG(logger_,
                   "Systematic data recovery error "
                   "(candidate={}, erasure_root={}): {}",
                   candidate_hash,
                   active.erasure_encoding_root,
                   res.error());
          incFullRecoveriesFinished("systematic_chunks", "invalid");
        } else {
          SL_TRACE(logger_,
                   "Data recovery from systematic chunks complete. "
                   "(candidate={}, erasure_root={})",
                   candidate_hash,
                   active.erasure_encoding_root);
          incFullRecoveriesFinished("systematic_chunks", "success");
          return done(lock, it, data);
        }
      }
      lock.unlock();

      SL_TRACE(logger_,
               "Candidate {}. "
               "Systematic chunk recovery has failed. "
               "Trying to do regular chunks recovery",
               candidate_hash);
      return regular_chunks_recovery_prepare(candidate_hash);
    }

    // Is it possible to collect all systematic chunks?
    bool is_possible_to_collect_systematic_chunks =
        systematic_chunk_count + active.chunks_active + active.order.size()
        >= active.chunks_required;

    if (not is_possible_to_collect_systematic_chunks) {
      active.systematic_chunk_failed = true;
      SL_TRACE(
          logger_,
          "Data recovery from systematic chunks is not possible. "
          "(candidate={} collected={} requested={} in-queue={} required={})",
          candidate_hash,
          systematic_chunk_count,
          active.chunks_active,
          active.order.size(),
          active.chunks_required);
      incFullRecoveriesFinished("systematic_chunks", "failure");
      lock.unlock();

      SL_TRACE(logger_,
               "Candidate {}. "
               "Systematic chunk recovery is not possible. "
               "Trying to do regular chunks recovery",
               candidate_hash);
      return regular_chunks_recovery_prepare(candidate_hash);
    }

    // Send requests
    auto max = std::min(kParallelRequests,
                        active.chunks_required - systematic_chunk_count);
    while (not active.order.empty() and active.chunks_active < max) {
      auto validator_index = active.order.back();
      active.order.pop_back();
      auto peer = query_audi_->get(active.discovery_keys[validator_index]);
      if (peer.has_value()) {
        ++active.chunks_active;
        SL_TRACE(logger_,
                 "Candidate {}. "
                 "Systematic recovery progress. "
                 "Asking validator #{} aka peer {}",
                 candidate_hash,
                 validator_index,
                 peer->id);
        active.queried.emplace(validator_index);
        send_fetch_chunk_request(
            peer->id,
            candidate_hash,
            active.val2chunk(validator_index),  // chunk_index
            &RecoveryImpl::systematic_chunks_recovery);
      } else {
        SL_TRACE(logger_,
                 "Candidate {}. "
                 "Systematic recovery progress. "
                 "PeerId of validator #{} is not discovered. Skipping... ",
                 candidate_hash,
                 validator_index);
      }
    }

    // No active request anymore for systematic chunks recovery
    if (active.chunks_active == 0) {
      active.systematic_chunk_failed = true;
      SL_TRACE(
          logger_,
          "Data recovery from systematic chunks is not possible. "
          "(candidate={} collected={} requested={} in-queue={} required={})",
          candidate_hash,
          systematic_chunk_count,
          active.chunks_active,
          active.order.size(),
          active.chunks_required);
      incFullRecoveriesFinished("systematic_chunks", "failure");
      lock.unlock();

      SL_TRACE(logger_,
               "Candidate {}. "
               "Systematic chunk recovery is not possible. "
               "Trying to do regular chunks recovery",
               candidate_hash);
      return regular_chunks_recovery_prepare(candidate_hash);
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

    // Update by existing chunks
    auto chunks = av_store_->getChunks(candidate_hash);
    for (auto &chunk : chunks) {
      if (std::ranges::find_if(
              active.chunks,
              [&](const auto &c) { return c.index == chunk.index; })
          == active.chunks.end()) {
        active.chunks.emplace_back(std::move(chunk));
      }
    }
    SL_TRACE(logger_,
             "Candidate {}. "
             "Regular recovery preparation. "
             "Already collected {} chunks",
             candidate_hash,
             active.chunks.size());

    // If existing chunks are already enough for regular chunk recovery
    if (active.chunks.size() >= active.chunks_required) {
      SL_TRACE(logger_,
               "Candidate {}. "
               "Regular recovery preparation. "
               "Already collected enough chunks ({} of {})",
               candidate_hash,
               active.chunks.size(),
               active.chunks_required);

      auto data_res = fromChunks(active.chunks_total, active.chunks);
      [[unlikely]] if (data_res.has_error()) {
        active.systematic_chunk_failed = true;
        SL_DEBUG(logger_,
                 "Data recovery error "
                 "(candidate={}, erasure_root={}): {}",
                 candidate_hash,
                 active.erasure_encoding_root,
                 data_res.error());
        incFullRecoveriesFinished("regular_chunks", "invalid");
      } else {
        auto &data = data_res.value();
        auto res = check(active, data);
        [[unlikely]] if (res.has_error()) {
          active.systematic_chunk_failed = true;
          SL_DEBUG(logger_,
                   "Data recovery error "
                   "(candidate={}, erasure_root={}): {}",
                   candidate_hash,
                   active.erasure_encoding_root,
                   res.error());
          incFullRecoveriesFinished("regular_chunks", "invalid");
          data_res = res.as_failure();
        } else {
          SL_TRACE(logger_,
                   "Data recovery from chunks complete. "
                   "(candidate={}, erasure_root={})",
                   candidate_hash,
                   active.erasure_encoding_root);
          incFullRecoveriesFinished("regular_chunks", "success");
        }
      }
      return done(lock, it, data_res);
    }

    // Refill request order by remaining validators
    for (size_t validator_index = 0; validator_index < active.chunks_total;
         ++validator_index) {
      // Filter queried
      if (active.queried.contains(validator_index)) {
        SL_TRACE(logger_,
                 "Candidate {}. "
                 "Regular recovery preparation. "
                 "Validator #{} has ignored as already queried",
                 candidate_hash,
                 validator_index);
        continue;
      }

      // Filter existing (only if mapping is not 1-to-1)
      if (active.val2chunk(0) != 0) {
        if (std::ranges::find_if(
                active.chunks,
                [chunk_index{active.val2chunk(validator_index)}](
                    network::ErasureChunk &c) {
                  return c.index == chunk_index;
                })
            != active.chunks.end()) {
          SL_TRACE(
              logger_,
              "Candidate {}. "
              "Regular recovery preparation. "
              "Validator #{} has ignored because such chunk already exists",
              candidate_hash,
              validator_index);
          continue;
        }
      }

      active.order.emplace_back(validator_index);
    }
    std::shuffle(active.order.begin(), active.order.end(), random_);
    SL_TRACE(logger_,
             "Candidate {}. "
             "Regular recovery preparation. "
             "{} validators in order for asking",
             candidate_hash,
             active.order.size());

    // Is it possible to collect enough chunks for recovery?
    auto is_possible_to_collect_required_chunks =
        active.chunks.size() + active.chunks_active + active.order.size()
        >= active.chunks_required;

    if (is_possible_to_collect_required_chunks) {
      lock.unlock();
      return regular_chunks_recovery(candidate_hash);
    }

    SL_TRACE(logger_,
             "Data recovery from chunks is not possible. "
             "(candidate={} collected={} requested={} in-queue={} required={})",
             candidate_hash,
             active.chunks.size(),
             active.chunks_active,
             active.order.size(),
             active.chunks_required);
    incFullRecoveriesFinished("regular_chunks", "failure");
    return done(lock, it, std::nullopt);
  }

  void RecoveryImpl::regular_chunks_recovery(
      const CandidateHash &candidate_hash) {
    Lock lock{mutex_};

    auto it = active_.find(candidate_hash);
    if (it == active_.end()) {
      return;
    }
    auto &active = it->second;

    // If existing chunks are already enough for regular chunk recovery
    if (active.chunks.size() >= active.chunks_required) {
      SL_TRACE(logger_,
               "Candidate {}. "
               "Regular recovery progress. "
               "Already collected enough chunks ({} of {})",
               candidate_hash,
               active.chunks.size(),
               active.chunks_required);

      auto data_res = fromChunks(active.chunks_total, active.chunks);
      [[unlikely]] if (data_res.has_error()) {
        SL_DEBUG(logger_,
                 "Data recovery error "
                 "(candidate={}, erasure_root={}): {}",
                 candidate_hash,
                 active.erasure_encoding_root,
                 data_res.error());
        incFullRecoveriesFinished("regular_chunks", "invalid");
      } else {
        auto &data = data_res.value();
        auto res = check(active, data);
        [[unlikely]] if (res.has_error()) {
          SL_DEBUG(logger_,
                   "Data recovery error "
                   "(candidate={}, erasure_root={}): {}",
                   candidate_hash,
                   active.erasure_encoding_root,
                   res.error());
          incFullRecoveriesFinished("regular_chunks", "invalid");
          data_res = res.as_failure();
        } else {
          SL_TRACE(logger_,
                   "Data recovery from chunks complete. "
                   "(candidate={}, erasure_root={})",
                   candidate_hash,
                   active.erasure_encoding_root);
          incFullRecoveriesFinished("regular_chunks", "success");
        }
      }
      return done(lock, it, data_res);
    }

    // Is it possible to collect enough chunks for recovery?
    auto is_possible_to_collect_required_chunks =
        active.chunks.size() + active.chunks_active + active.order.size()
        >= active.chunks_required;

    if (not is_possible_to_collect_required_chunks) {
      SL_TRACE(
          logger_,
          "Data recovery from chunks is not possible. "
          "(candidate={} collected={} requested={} in-queue={} required={})",
          candidate_hash,
          active.chunks.size(),
          active.chunks_active,
          active.order.size(),
          active.chunks_required);
      incFullRecoveriesFinished("regular_chunks", "failure");
      return done(lock, it, std::nullopt);
    }

    // Send requests
    auto max = std::min(kParallelRequests,
                        active.chunks_required - active.chunks.size());
    while (not active.order.empty() and active.chunks_active < max) {
      auto validator_index = active.order.back();
      active.order.pop_back();
      auto peer = query_audi_->get(active.discovery_keys[validator_index]);
      if (peer.has_value()) {
        ++active.chunks_active;
        SL_TRACE(logger_,
                 "Candidate {}. "
                 "Regular recovery progress. "
                 "Asking validator #{} aka peer {}",
                 candidate_hash,
                 validator_index,
                 peer->id);
        active.queried.emplace(validator_index);
        send_fetch_chunk_request(peer->id,
                                 candidate_hash,
                                 active.val2chunk(validator_index),
                                 &RecoveryImpl::regular_chunks_recovery);
      } else {
        SL_TRACE(logger_,
                 "Candidate {}. "
                 "Regular recovery progress. "
                 "PeerId of validator #{} is not discovered. Skipping... ",
                 candidate_hash,
                 validator_index);
      }
    }

    // No active request anymore for regular chunks recovery
    if (active.chunks_active == 0) {
      SL_TRACE(
          logger_,
          "Data recovery from chunks is not possible. "
          "(candidate={} collected={} requested={} in-queue={} required={})",
          candidate_hash,
          active.chunks.size(),
          active.chunks_active,
          active.order.size(),
          active.chunks_required);
      incFullRecoveriesFinished("regular_chunks", "failure");
      return done(lock, it, std::nullopt);
    }
  }

  // Fetch available data protocol communication
  void RecoveryImpl::send_fetch_available_data_request(
      const libp2p::PeerId &peer_id,
      const CandidateHash &candidate_hash,
      SelfCb next_iteration) {
    SL_TRACE(logger_,
             "Candidate {}. "
             "Send available data request to peer {}",
             candidate_hash,
             peer_id);

    router_->getFetchAvailableDataProtocol()->doRequest(
        peer_id,
        candidate_hash,
        [weak{weak_from_this()}, candidate_hash, peer_id, next_iteration](
            outcome::result<network::FetchAvailableDataResponse> response_res) {
          if (auto self = weak.lock()) {
            self->handle_fetch_available_data_response(peer_id,
                                                       candidate_hash,
                                                       std::move(response_res),
                                                       next_iteration);
          }
        });
  }

  void RecoveryImpl::handle_fetch_available_data_response(
      const libp2p::PeerId &peer_id,
      const CandidateHash &candidate_hash,
      outcome::result<network::FetchAvailableDataResponse> response_res,
      SelfCb next_iteration) {
    Lock lock{mutex_};

    auto it = active_.find(candidate_hash);
    if (it == active_.end()) {
      return;
    }

    auto &active = it->second;

    if (response_res.has_value()) {
      if (auto data = boost::get<AvailableData>(&response_res.value())) {
        auto res = check(active, *data);
        [[unlikely]] if (res.has_error()) {
          SL_TRACE(logger_,
                   "Candidate {}. "
                   "Peer {} returns INVALID data: {}",
                   candidate_hash,
                   peer_id,
                   res.error());
          incFullRecoveriesFinished("full_from_backers", "invalid");
        } else {
          SL_TRACE(logger_,
                   "Candidate {}. "
                   "Peer {} returns valid data",
                   candidate_hash,
                   peer_id);
          incFullRecoveriesFinished("full_from_backers", "success");
          return done(lock, it, std::move(*data));
        }
      } else {
        SL_TRACE(logger_,
                 "Candidate {}. "
                 "Peer {} returns Empty for available data request",
                 candidate_hash,
                 peer_id);
      }
    } else {
      SL_TRACE(logger_,
               "Candidate {}. "
               "Fetch available data request to peer {} was failed: {}",
               candidate_hash,
               peer_id,
               response_res.error());
    }

    lock.unlock();

    (this->*next_iteration)(candidate_hash);
  }

  void RecoveryImpl::send_fetch_chunk_request(
      const libp2p::PeerId &peer_id,
      const CandidateHash &candidate_hash,
      ChunkIndex chunk_index,
      SelfCb next_iteration) {
    auto peer_state = [&]() {
      auto res = pm_->getPeerState(peer_id);
      if (!res) {
        SL_TRACE(logger_,
                 "No PeerState of peer {}. Default one has created",
                 peer_id);
        res = pm_->createDefaultPeerState(peer_id);
      }
      return res;
    }();

    auto req_chunk_version = peer_state->get().req_chunk_version.value_or(
        network::ReqChunkVersion::V1_obsolete);

    SL_TRACE(logger_,
             "Candidate {}. "
             "Send chunk #{} request to peer {}",
             candidate_hash,
             chunk_index,
             peer_id);

    switch (req_chunk_version) {
      case network::ReqChunkVersion::V2: {
        router_->getFetchChunkProtocol()->doRequest(
            peer_id,
            {candidate_hash, chunk_index},
            [weak{weak_from_this()}, candidate_hash, peer_id, next_iteration](
                outcome::result<network::FetchChunkResponse> response_res) {
              if (auto self = weak.lock()) {
                self->handle_fetch_chunk_response(peer_id,
                                                  candidate_hash,
                                                  std::move(response_res),
                                                  next_iteration);
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
                      [](network::Empty &empty) -> network::FetchChunkResponse {
                        return empty;
                      },
                      [&](network::ChunkObsolete &chunk_obsolete)
                          -> network::FetchChunkResponse {
                        return network::Chunk{
                            .data = std::move(chunk_obsolete.data),
                            .chunk_index = chunk_index,
                            .proof = std::move(chunk_obsolete.proof),
                        };
                      });
                  self->handle_fetch_chunk_response(peer_id,
                                                    candidate_hash,
                                                    std::move(response),
                                                    next_iteration);
                } else {
                  self->handle_fetch_chunk_response(peer_id,
                                                    candidate_hash,
                                                    response_res.as_failure(),
                                                    next_iteration);
                }
              }
            });
      } break;
      default:
        UNREACHABLE;
    }
  }

  void RecoveryImpl::handle_fetch_chunk_response(
      const libp2p::PeerId &peer_id,
      const CandidateHash &candidate_hash,
      outcome::result<network::FetchChunkResponse> response_res,
      SelfCb next_iteration) {
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
            .chunk = std::move(chunk->data),
            .index = chunk->chunk_index,
            .proof = std::move(chunk->proof),
        };
        auto res = checkTrieProof(erasure_chunk, active.erasure_encoding_root);
        if (res.has_value()) {
          SL_TRACE(logger_,
                   "Candidate {}. "
                   "Peer {} returns valid chunk #{}",
                   candidate_hash,
                   peer_id,
                   chunk->chunk_index);
          active.chunks.emplace_back(std::move(erasure_chunk));
        } else {
          SL_TRACE(logger_,
                   "Candidate {}. "
                   "Peer {} returns INVALID chunk #{}: {}",
                   candidate_hash,
                   peer_id,
                   chunk->chunk_index,
                   res.error());
        }
      } else {
        SL_TRACE(logger_,
                 "Candidate {}. "
                 "Peer {} returns Empty for chunk request",
                 candidate_hash,
                 peer_id);
      }
    } else {
      SL_TRACE(logger_,
               "Candidate {}. "
               "Fetch chunk request to peer {} was failed: {}",
               candidate_hash,
               peer_id,
               response_res.error());
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
      const std::optional<outcome::result<AvailableData>> &result_op) {
    if (result_op.has_value()) {
      auto &result = result_op.value();
      cached_.emplace(it->first, result);
    }

    auto node = active_.extract(it);

    SL_TRACE(logger_,
             "Candidate {}. "
             "Stop recovery. "
             "has result: {}, "
             "has data: {}",
             it->first,
             result_op.has_value(),
             result_op.has_value() ? result_op.value().has_value() : false);

    lock.unlock();
    for (auto &cb : node.mapped().cb) {
      cb(result_op);
    }
  }
}  // namespace kagome::parachain
