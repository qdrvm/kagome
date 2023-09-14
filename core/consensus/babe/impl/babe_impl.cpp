/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/babe_impl.hpp"

#include <boost/range/adaptor/transformed.hpp>

#include "application/app_configuration.hpp"
#include "application/app_state_manager.hpp"
#include "authorship/proposer.hpp"
#include "blockchain/block_tree.hpp"
#include "blockchain/block_tree_error.hpp"
#include "blockchain/digest_tracker.hpp"
#include "consensus/babe/babe_config_repository.hpp"
#include "consensus/babe/babe_error.hpp"
#include "consensus/babe/babe_lottery.hpp"
#include "consensus/babe/babe_util.hpp"
#include "consensus/babe/consistency_keeper.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "consensus/babe/impl/backoff.hpp"
#include "consensus/babe/impl/threshold_util.hpp"
#include "consensus/grandpa/justification_observer.hpp"
#include "crypto/crypto_store/session_keys.hpp"
#include "crypto/sr25519_provider.hpp"
#include "dispute_coordinator/dispute_coordinator.hpp"
#include "dispute_coordinator/types.hpp"
#include "log/formatters/peer_id.hpp"
#include "metrics/histogram_timer.hpp"
#include "network/block_announce_transmitter.hpp"
#include "network/synchronizer.hpp"
#include "network/types/collator_messages.hpp"
#include "network/warp/protocol.hpp"
#include "network/warp/sync.hpp"
#include "parachain/parachain_inherent_data.hpp"
#include "runtime/runtime_api/core.hpp"
#include "runtime/runtime_api/offchain_worker_api.hpp"
#include "storage/changes_trie/impl/storage_changes_tracker_impl.hpp"
#include "storage/trie/serialization/ordered_trie_hash.hpp"
#include "storage/trie/trie_storage.hpp"
#include "utils/thread_pool.hpp"

namespace {
  constexpr const char *kIsMajorSyncing = "kagome_sub_libp2p_is_major_syncing";
  constexpr const char *kIsRelayChainValidator =
      "kagome_node_is_active_validator";
}  // namespace

using namespace std::literals::chrono_literals;

namespace kagome::consensus::babe {
  using ParachainInherentData = parachain::ParachainInherentData;
  using SyncMethod = application::AppConfiguration::SyncMethod;

  metrics::HistogramTimer metric_block_proposal_time{
      "kagome_proposer_block_constructed",
      "Time taken to construct new block",
      {0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1, 2.5, 5, 10},
  };

  inline auto fmtRemains(const BabeClock &clock, BabeTimePoint time) {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::max(time - clock.now(), {}));
    return fmt::format("{:.2f} sec", ms.count() / 1000.0);
  }

  BabeImpl::BabeImpl(
      const application::AppConfiguration &app_config,
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<BabeLottery> lottery,
      std::shared_ptr<BabeConfigRepository> babe_config_repo,
      const ThreadPool &thread_pool,
      std::shared_ptr<authorship::Proposer> proposer,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<network::BlockAnnounceTransmitter>
          block_announce_transmitter,
      std::shared_ptr<crypto::Sr25519Provider> sr25519_provider,
      std::shared_ptr<crypto::SessionKeys> session_keys,
      std::shared_ptr<clock::SystemClock> clock,
      std::shared_ptr<crypto::Hasher> hasher,
      std::unique_ptr<clock::Timer> timer,
      std::shared_ptr<blockchain::DigestTracker> digest_tracker,
      std::shared_ptr<network::WarpSync> warp_sync,
      LazySPtr<network::WarpProtocol> warp_protocol,
      std::shared_ptr<consensus::grandpa::JustificationObserver>
          justification_observer,
      std::shared_ptr<network::Synchronizer> synchronizer,
      std::shared_ptr<BabeUtil> babe_util,
      std::shared_ptr<parachain::BitfieldStore> bitfield_store,
      std::shared_ptr<parachain::BackingStore> backing_store,
      primitives::events::StorageSubscriptionEnginePtr storage_sub_engine,
      primitives::events::ChainSubscriptionEnginePtr chain_events_engine,
      std::shared_ptr<runtime::OffchainWorkerApi> offchain_worker_api,
      std::shared_ptr<runtime::Core> core,
      std::shared_ptr<ConsistencyKeeper> consistency_keeper,
      std::shared_ptr<storage::trie::TrieStorage> trie_storage,
      primitives::events::BabeStateSubscriptionEnginePtr babe_status_observable,
      std::shared_ptr<dispute::DisputeCoordinator> dispute_coordinator)
      : sync_method_(app_config.syncMethod()),
        app_config_validator_{app_config.roles().flags.authority != 0},
        app_state_manager_(app_state_manager),
        lottery_{std::move(lottery)},
        babe_config_repo_{std::move(babe_config_repo)},
        io_context_{thread_pool.io_context()},
        proposer_{std::move(proposer)},
        block_tree_{std::move(block_tree)},
        block_announce_transmitter_{std::move(block_announce_transmitter)},
        clock_{std::move(clock)},
        hasher_{std::move(hasher)},
        sr25519_provider_{std::move(sr25519_provider)},
        session_keys_{std::move(session_keys)},
        timer_{std::move(timer)},
        digest_tracker_(std::move(digest_tracker)),
        warp_sync_{std::move(warp_sync)},
        warp_protocol_{std::move(warp_protocol)},
        justification_observer_{std::move(justification_observer)},
        synchronizer_(std::move(synchronizer)),
        babe_util_(std::move(babe_util)),
        bitfield_store_{std::move(bitfield_store)},
        backing_store_{std::move(backing_store)},
        storage_sub_engine_{std::move(storage_sub_engine)},
        chain_events_engine_(std::move(chain_events_engine)),
        chain_sub_([&] {
          BOOST_ASSERT(chain_events_engine_ != nullptr);
          return std::make_shared<primitives::events::ChainEventSubscriber>(
              chain_events_engine_);
        }()),
        offchain_worker_api_(std::move(offchain_worker_api)),
        runtime_core_(std::move(core)),
        consistency_keeper_(std::move(consistency_keeper)),
        trie_storage_(std::move(trie_storage)),
        babe_status_observable_(std::move(babe_status_observable)),
        dispute_coordinator_(std::move(dispute_coordinator)),
        log_{log::createLogger("Babe", "babe")},
        telemetry_{telemetry::createTelemetryService()} {
    BOOST_ASSERT(app_state_manager_);
    BOOST_ASSERT(lottery_);
    BOOST_ASSERT(babe_config_repo_);
    BOOST_ASSERT(proposer_);
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(block_announce_transmitter_);
    BOOST_ASSERT(sr25519_provider_);
    BOOST_ASSERT(clock_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(timer_);
    BOOST_ASSERT(log_);
    BOOST_ASSERT(digest_tracker_);
    BOOST_ASSERT(synchronizer_);
    BOOST_ASSERT(babe_util_);
    BOOST_ASSERT(offchain_worker_api_);
    BOOST_ASSERT(runtime_core_);
    BOOST_ASSERT(consistency_keeper_);
    BOOST_ASSERT(trie_storage_);
    BOOST_ASSERT(babe_status_observable_);
    BOOST_ASSERT(dispute_coordinator_);

    BOOST_ASSERT(app_state_manager);

    // Register metrics
    metrics_registry_->registerGaugeFamily(
        kIsMajorSyncing, "Whether the node is performing a major sync or not.");
    metric_is_major_syncing_ =
        metrics_registry_->registerGaugeMetric(kIsMajorSyncing);
    metric_is_major_syncing_->set(!was_synchronized_);
    metrics_registry_->registerGaugeFamily(
        kIsRelayChainValidator,
        "Tracks if the validator is in the active set. Updates at session "
        "boundary.");
    metric_is_relaychain_validator_ =
        metrics_registry_->registerGaugeMetric(kIsRelayChainValidator);
    metric_is_relaychain_validator_->set(false);

    app_state_manager_->takeControl(*this);
  }

  namespace {
    std::tuple<std::chrono::microseconds,
               std::chrono::microseconds,
               std::chrono::microseconds>
    estimateSyncDuration(size_t lag_slots, BabeDuration slot_duration) {
      // WARP: n * header_loading / k + state_loading + lag * block_execution
      //       {               catchup              }
      // FAST: n * header_loading + state_loading + lag' * block_execution
      //       {             catchup'           }
      // FULL: n * block_execution + lag" * block_execution
      //       {     catchup"    }

#ifdef NDEBUG
      auto block_execution =
          std::chrono::microseconds(650'000);  // 0.65s (wavm)
#else
      auto block_execution = std::chrono::microseconds(50'000);  // 50ms (wavm)
#endif
      auto header_loading = std::chrono::microseconds(5'000);        // 5ms
      auto state_loading = std::chrono::microseconds(1800'000'000);  // 0.5hr
      auto warp_proportion = 10'000;  // ~one set id change for each 10k blocks

      auto warp_catchup = header_loading * lag_slots  // time of headers loading
                            / warp_proportion  // part of requesting headers
                        + state_loading;       // time of state loading
      auto fast_catchup = header_loading * lag_slots  // time of headers loading
                        + block_execution * 512  // execute non-finalized blocks
                        + state_loading;         // time of state loading
      auto full_catchup = block_execution * lag_slots;  // execute all blocks

      auto warp_lag = warp_catchup / slot_duration;
      auto fast_lag = fast_catchup / slot_duration;
      auto full_lag = full_catchup / slot_duration;

      auto warp_sync_duration = warp_catchup + block_execution * warp_lag;
      auto fast_sync_duration = fast_catchup + block_execution * fast_lag;
      auto full_sync_duration = full_catchup + block_execution * full_lag;

      return {warp_sync_duration, fast_sync_duration, full_sync_duration};
    }
  }  // namespace

  bool BabeImpl::prepare() {
    updateSlot(clock_->now());

    auto best_block_header_res = block_tree_->getBlockHeader(best_block_.hash);
    if (best_block_header_res.has_error()) {
      SL_CRITICAL(log_,
                  "Can't get header of best block ({}): {}",
                  best_block_,
                  best_block_header_res.error());
      return false;
    }
    const auto &best_block_header = best_block_header_res.value();
    const auto &state_root = best_block_header.state_root;

    // Calculate lag our best block by slots
    BabeSlotNumber lag_slots = 0;
    if (auto slot_res = getBabeSlot(best_block_header)) {
      lag_slots = babe_util_->timeToSlot(clock_->now()) - slot_res.value();
    }

    auto &&[warp_sync_duration, fast_sync_duration, full_sync_duration] =
        estimateSyncDuration(lag_slots, babe_config_repo_->slotDuration());

    bool allow_warp_sync_for_auto = false;  // should it select warp for auto

    // Check if target block does not have state (full sync not available)
    bool full_sync_available = true;
    if (auto res = trie_storage_->getEphemeralBatchAt(state_root);
        not res.has_value()) {
      if (sync_method_ == SyncMethod::Full) {
        SL_WARN(log_, "Can't get state of best block: {}", res.error());
        SL_CRITICAL(log_,
                    "Try restart at least once with `--sync Fast' CLI arg");
        return false;
      }
      full_sync_available = false;
    }

    switch (sync_method_) {
      case SyncMethod::Auto:
        if (full_sync_duration < fast_sync_duration and full_sync_available) {
          SL_INFO(log_, "Sync mode auto: Full sync selected");
          sync_method_ = SyncMethod::Full;
        } else if (fast_sync_duration < warp_sync_duration
                   or not allow_warp_sync_for_auto) {
          SL_INFO(log_, "Sync mode auto: Fast sync selected");
          sync_method_ = SyncMethod::Fast;
        } else {
          SL_INFO(log_, "Sync mode auto: Warp sync selected");
          sync_method_ = SyncMethod::Warp;
        }
        break;

      case SyncMethod::Full:
        if (fast_sync_duration < full_sync_duration) {
          SL_INFO(log_,
                  "Fast sync would be faster than Full sync that was selected");
        } else if (warp_sync_duration < full_sync_duration) {
          SL_INFO(log_,
                  "Warp sync would be faster than Full sync that was selected");
        }
        break;

      case SyncMethod::FastWithoutState:
        break;

      case SyncMethod::Fast:
        if (full_sync_duration < fast_sync_duration and full_sync_available) {
          SL_INFO(log_,
                  "Full sync would be faster than Fast sync that was selected");
        } else if (warp_sync_duration < fast_sync_duration) {
          SL_INFO(log_,
                  "Warp sync would be faster than Fast sync that was selected");
        }
        break;

      case SyncMethod::Warp:
        if (full_sync_duration < warp_sync_duration and full_sync_available) {
          SL_INFO(log_,
                  "Full sync would be faster than Warp sync that was selected");
        } else if (fast_sync_duration < warp_sync_duration) {
          SL_INFO(log_,
                  "Fast sync would be faster than Warp sync that was selected");
        }
        break;
    }

    chain_sub_->subscribe(chain_sub_->generateSubscriptionSetId(),
                          primitives::events::ChainEventType::kFinalizedHeads);
    chain_sub_->setCallback([wp = weak_from_this()](
                                subscription::SubscriptionSetId,
                                auto &&,
                                primitives::events::ChainEventType type,
                                const primitives::events::ChainEventParams
                                    &event) {
      if (type == primitives::events::ChainEventType::kFinalizedHeads) {
        if (auto self = wp.lock()) {
          if (self->current_state_ != Babe::State::HEADERS_LOADING
              and self->current_state_ != Babe::State::STATE_LOADING) {
            const auto &header =
                boost::get<primitives::events::HeadsEventParams>(event).get();
            auto hash =
                self->hasher_->blake2b_256(scale::encode(header).value());

            auto version_res = self->runtime_core_->version(hash);
            if (version_res.has_value()) {
              auto &version = version_res.value();
              if (not self->actual_runtime_version_.has_value()
                  || self->actual_runtime_version_ != version) {
                self->actual_runtime_version_ = version;
                self->chain_events_engine_->notify(
                    primitives::events::ChainEventType::
                        kFinalizedRuntimeVersion,
                    version);
              }
            }
          }
        }
      }
    });

    return true;
  }

  bool BabeImpl::start() {
    updateSlot(clock_->now());

    SL_DEBUG(log_, "Babe is starting with syncing from block {}", best_block_);

    SL_DEBUG(log_,
             "Starting in epoch {} and slot {}",
             current_epoch_.epoch_number,
             current_epoch_.start_slot);

    auto babe_config =
        babe_config_repo_->config(best_block_, current_epoch_.epoch_number);
    if (not babe_config and sync_method_ != SyncMethod::Warp) {
      SL_CRITICAL(
          log_,
          "Can't obtain digest of epoch {} from block tree for block {}",
          current_epoch_.epoch_number,
          best_block_);
      return false;
    }
    if (babe_config) {
      const auto &authorities = babe_config.value()->authorities;
      if (authorities.size() == 1
          && session_keys_->getBabeKeyPair(authorities)) {
        SL_INFO(log_, "Starting single validating node.");
        onSynchronized();
        return true;
      }
    }

    switch (sync_method_) {
      case SyncMethod::Full:
        current_state_ = State::WAIT_REMOTE_STATUS;
        break;

      case SyncMethod::Fast:
      case SyncMethod::Warp:
      case SyncMethod::FastWithoutState: {
        current_state_ = State::HEADERS_LOADING;
        babe_status_observable_->notify(
            primitives::events::BabeStateEventType::kSyncState, current_state_);
      } break;

      case SyncMethod::Auto:
        UNREACHABLE;  // It must be rewritten in prepare stage
    }

    return true;
  }

  bool BabeImpl::updateSlot(BabeTimePoint now) {
    best_block_ = block_tree_->bestLeaf();
    current_slot_ = babe_util_->timeToSlot(now);
    auto epoch_res =
        babe_util_->slotToEpochDescriptor(best_block_, current_slot_);
    if (not epoch_res) {
      SL_ERROR(log_, "updateSlot: can't get epoch: {}", epoch_res.error());
      return false;
    }
    current_epoch_ = epoch_res.value();
    return true;
  }

  void BabeImpl::runEpoch() {
    bool already_active = false;
    if (not active_.compare_exchange_strong(already_active, true)) {
      return;
    }
    runSlot();
  }

  Babe::State BabeImpl::getCurrentState() const {
    return current_state_;
  }

  void BabeImpl::onBlockAnnounceHandshake(
      const libp2p::peer::PeerId &peer_id,
      const network::BlockAnnounceHandshake &handshake) {
    // If state is loading, just to ping of loading
    if (current_state_ == Babe::State::STATE_LOADING) {
      startStateSyncing(peer_id);
      return;
    }

    if (warpSync(peer_id, handshake.best_block.number)) {
      return;
    }

    const auto &last_finalized_block = block_tree_->getLastFinalized();

    auto current_best_block_res =
        block_tree_->getBestContaining(last_finalized_block.hash, std::nullopt);
    BOOST_ASSERT(current_best_block_res.has_value());
    const auto &current_best_block = current_best_block_res.value();

    if (current_best_block == handshake.best_block) {
      if (current_state_ == Babe::State::HEADERS_LOADING) {
        current_state_ = Babe::State::HEADERS_LOADED;
        babe_status_observable_->notify(
            primitives::events::BabeStateEventType::kSyncState, current_state_);
        startStateSyncing(peer_id);
      } else if (current_state_ == Babe::State::CATCHING_UP
                 or current_state_ == Babe::State::WAIT_REMOTE_STATUS) {
        onCaughtUp(current_best_block);
      }
      return;
    }

    // Remote peer is lagged
    if (handshake.best_block.number <= last_finalized_block.number) {
      return;
    }

    startCatchUp(peer_id, handshake.best_block);
  }

  void BabeImpl::onBlockAnnounce(const libp2p::peer::PeerId &peer_id,
                                 const network::BlockAnnounce &announce) {
    // If state is loading, just to ping of loading
    if (current_state_ == Babe::State::STATE_LOADING) {
      startStateSyncing(peer_id);
      return;
    }

    if (warpSync(peer_id, announce.header.number)) {
      return;
    }

    const auto &last_finalized_block = block_tree_->getLastFinalized();

    auto current_best_block_res =
        block_tree_->getBestContaining(last_finalized_block.hash, std::nullopt);
    BOOST_ASSERT(current_best_block_res.has_value());
    const auto &current_best_block = current_best_block_res.value();

    // Skip obsoleted announce
    if (announce.header.number < current_best_block.number) {
      return;
    }

    // Start catching up if gap recognized
    if (current_state_ == Babe::State::SYNCHRONIZED
        or current_state_ == Babe::State::HEADERS_LOADED) {
      if (announce.header.number > current_best_block.number + 1) {
        auto block_hash =
            hasher_->blake2b_256(scale::encode(announce.header).value());
        const primitives::BlockInfo announced_block(announce.header.number,
                                                    block_hash);
        startCatchUp(peer_id, announced_block);
        return;
      }
    }

    // Received announce that has the same block number as ours best,
    // or greater by one. Using of simple way to load block
    synchronizer_->syncByBlockHeader(
        announce.header,
        peer_id,
        [wp = weak_from_this(), announce = announce, peer_id](
            outcome::result<primitives::BlockInfo> block_res) mutable {
          if (auto self = wp.lock()) {
            if (block_res.has_error()) {
              return;
            }
            const auto &block = block_res.value();

            // Headers are loaded; Start sync of state
            if (self->current_state_ == Babe::State::HEADERS_LOADING) {
              self->current_state_ = Babe::State::HEADERS_LOADED;
              self->babe_status_observable_->notify(
                  primitives::events::BabeStateEventType::kSyncState,
                  self->current_state_);
              self->startStateSyncing(peer_id);
              return;
            }

            // Caught up some block, possible block of current slot
            if (self->current_state_ == Babe::State::CATCHING_UP
                or self->current_state_ == State::WAIT_REMOTE_STATUS) {
              self->onCaughtUp(block);
            }

            // Synced
            if (self->current_state_ == Babe::State::SYNCHRONIZED) {
              // Set actual block status
              announce.state = block == self->block_tree_->bestLeaf()
                                 ? network::BlockState::Best
                                 : network::BlockState::Normal;
              // Propagate announce
              self->block_announce_transmitter_->blockAnnounce(
                  std::move(announce));
              return;
            }
          }
        });
  }

  bool BabeImpl::warpSync(const libp2p::peer::PeerId &peer_id,
                          primitives::BlockNumber block_number) {
    if (current_state_ != State::HEADERS_LOADING) {
      return false;
    }
    if (sync_method_ != SyncMethod::Warp) {
      return false;
    }
    auto target = warp_sync_->request();
    if (not target) {
      current_state_ = State::HEADERS_LOADED;
      startStateSyncing(peer_id);
      return true;
    }
    if (block_number <= target->number) {
      return true;
    }
    if (warp_sync_busy_) {
      return true;
    }
    warp_sync_busy_ = true;
    auto cb = [=, weak{weak_from_this()}](
                  outcome::result<network::WarpSyncProof> _res) {
      auto self = weak.lock();
      if (not self) {
        return;
      }
      if (not _res) {
        self->warp_sync_busy_ = false;
        return;
      }
      auto &res = _res.value();
      self->warp_sync_->onResponse(res);
      self->warp_sync_busy_ = false;
      self->warpSync(peer_id, block_number);
    };
    warp_protocol_.get()->doRequest(peer_id, target->hash, std::move(cb));
    return true;
  }

  void BabeImpl::startCatchUp(const libp2p::peer::PeerId &peer_id,
                              const primitives::BlockInfo &target_block) {
    BOOST_ASSERT(current_state_ != Babe::State::STATE_LOADING);

    // synchronize missing blocks with their bodies
    auto is_ran = synchronizer_->syncByBlockInfo(
        target_block,
        peer_id,
        [wp = weak_from_this(), block = target_block, peer_id](
            outcome::result<primitives::BlockInfo> res) {
          if (auto self = wp.lock()) {
            if (res.has_error()) {
              SL_DEBUG(self->log_,
                       "Catching up {} to block {} is failed: {}",
                       peer_id,
                       block,
                       res.error());
              return;
            }

            SL_DEBUG(self->log_,
                     "Catching up {} to block {} is going; on block {} now",
                     peer_id,
                     block,
                     res.value());
          }
        },
        false);

    if (is_ran) {
      SL_VERBOSE(
          log_, "Catching up {} to block {} is ran", peer_id, target_block);
      if (current_state_ == State::HEADERS_LOADED) {
        current_state_ = State::HEADERS_LOADING;
        babe_status_observable_->notify(
            primitives::events::BabeStateEventType::kSyncState, current_state_);
      } else if (current_state_ == State::WAIT_BLOCK_ANNOUNCE
                 or current_state_ == State::WAIT_REMOTE_STATUS
                 or current_state_ == State::SYNCHRONIZED) {
        current_state_ = State::CATCHING_UP;
        babe_status_observable_->notify(
            primitives::events::BabeStateEventType::kSyncState, current_state_);
      }
    }
  }

  void BabeImpl::startStateSyncing(const libp2p::peer::PeerId &peer_id) {
    BOOST_ASSERT(current_state_ == Babe::State::HEADERS_LOADED
                 or current_state_ == Babe::State::STATE_LOADING);
    if (current_state_ != Babe::State::HEADERS_LOADED
        and current_state_ != Babe::State::STATE_LOADING) {
      SL_WARN(log_, "Syncing of state can not be start: Bad state of babe");
      return;
    }

    current_state_ = Babe::State::STATE_LOADING;
    babe_status_observable_->notify(
        primitives::events::BabeStateEventType::kSyncState, current_state_);

    auto best_block =
        block_tree_->getBlockHeader(block_tree_->bestLeaf().hash).value();
    if (trie_storage_->getEphemeralBatchAt(best_block.state_root)) {
      current_state_ = Babe::State::CATCHING_UP;
      return;
    }

    if (sync_method_ == SyncMethod::FastWithoutState) {
      if (app_state_manager_->state()
          != application::AppStateManager::State::ShuttingDown) {
        SL_INFO(log_,
                "Stateless fast sync is finished on block {}; "
                "Application is stopping",
                block_tree_->bestLeaf());
        log_->flush();
        app_state_manager_->shutdown();
      }
      return;
    }

    // Switch to last finalized to have a state on it
    auto block_at_state = block_tree_->getLastFinalized();

    SL_DEBUG(log_,
             "Rolling back non-finalized blocks. Last known finalized is {}",
             block_at_state);

    // Next do-while-loop serves for removal non finalized blocks
    bool affected;
    do {
      affected = false;

      auto block_tree_leaves = block_tree_->getLeaves();

      for (const auto &hash : block_tree_leaves) {
        if (hash == block_at_state.hash) {
          continue;
        }

        auto header_res = block_tree_->getBlockHeader(hash);
        if (header_res.has_error()) {
          SL_CRITICAL(log_,
                      "Can't get header of one of removing leave_block: {}",
                      header_res.error());
          continue;
        }

        const auto &header = header_res.value();

        // Block below last finalized must not being. Don't touch just in case
        if (header.number < block_at_state.number) {
          continue;
        }

        std::ignore = consistency_keeper_->start(
            primitives::BlockInfo(header.number, hash));

        affected = true;
      }
    } while (affected);

    SL_TRACE(log_,
             "Trying to sync state on block {} from {}",
             block_at_state,
             peer_id);

    synchronizer_->syncState(
        peer_id,
        block_at_state,
        [wp = weak_from_this(), block_at_state, peer_id](auto res) mutable {
          if (auto self = wp.lock()) {
            if (res.has_error()) {
              SL_WARN(self->log_,
                      "Syncing of state with {} on block {} is failed: {}",
                      peer_id,
                      block_at_state,
                      res.error());
              return;
            }

            self->justification_observer_->reload();
            self->block_tree_->notifyBestAndFinalized();

            SL_INFO(self->log_,
                    "State on block {} is synced successfully",
                    block_at_state);
            self->current_state_ = Babe::State::CATCHING_UP;
            self->babe_status_observable_->notify(
                primitives::events::BabeStateEventType::kSyncState,
                self->current_state_);
          }
        });
  }

  void BabeImpl::onCaughtUp(const primitives::BlockInfo &block) {
    SL_INFO(log_, "Caught up block {}", block);

    if (not was_synchronized_) {
      auto header_opt = block_tree_->getBlockHeader(block.hash);
      BOOST_ASSERT_MSG(header_opt.has_value(), "Just added block; deq");
      auto slot_res = getBabeSlot(header_opt.value());
      if (slot_res.has_value()) {
        if (babe_util_->timeToSlot(clock_->now()) > slot_res.value() + 1) {
          current_state_ = Babe::State::WAIT_REMOTE_STATUS;
          babe_status_observable_->notify(
              primitives::events::BabeStateEventType::kSyncState,
              current_state_);
          return;
        }
      }
    }

    onSynchronized();
  }

  void BabeImpl::onSynchronized() {
    if (not was_synchronized_) {
      was_synchronized_ = true;

      telemetry_->notifyWasSynchronized();
    }
    metric_is_major_syncing_->set(!was_synchronized_);
    current_state_ = Babe::State::SYNCHRONIZED;

    babe_status_observable_->notify(
        primitives::events::BabeStateEventType::kSyncState, current_state_);

    if (not active_) {
      best_block_ = block_tree_->bestLeaf();
      SL_DEBUG(log_, "Babe is synchronized on block {}", best_block_);
      runEpoch();
    }
  }

  bool BabeImpl::wasSynchronized() const {
    return was_synchronized_;
  }

  void BabeImpl::runSlot() {
    auto now = clock_->now();
    if (not updateSlot(now)) {
      timer_->expiresAt(babe_util_->slotFinishTime(current_slot_));
      timer_->asyncWait([this](boost::system::error_code ec) {
        if (ec) {
          log_->error("error happened while waiting on the timer: {}", ec);
          return;
        }
        runSlot();
      });
      return;
    }

    SL_VERBOSE(log_,
               "Starting a slot {} in epoch {} (remains {})",
               current_slot_,
               current_epoch_.epoch_number,
               fmtRemains(*clock_, babe_util_->slotFinishTime(current_slot_)));

    processSlot(now);
  }

  void BabeImpl::processSlot(clock::SystemClock::TimePoint slot_timestamp) {
    if (best_block_.number != 0) {
      auto parent_header =
          block_tree_->getBlockHeader(best_block_.hash).value();
      auto parent_slot = getBabeSlot(parent_header).value();
      if (parent_slot > current_slot_) {
        SL_ERROR(log_,
                 "best block {} from future, current slot {}, clock is lagging",
                 best_block_,
                 current_slot_);
        return;
      }
      if (parent_slot == current_slot_) {
        // fork or wait next slot
        SL_INFO(log_,
                "concurrent best block {}, current slot {}, will fork",
                best_block_,
                current_slot_);
        best_block_ = *parent_header.parentInfo();
      }
    }

    auto babe_config_opt =
        babe_config_repo_->config(best_block_, current_epoch_.epoch_number);
    if (babe_config_opt) {
      auto &babe_config = *babe_config_opt.value();
      auto keypair = session_keys_->getBabeKeyPair(babe_config.authorities);
      if (not keypair) {
        metric_is_relaychain_validator_->set(false);
        if (app_config_validator_) {
          SL_VERBOSE(log_,
                     "Authority not known, skipping slot processing. "
                     "Probably authority list has changed.");
        }
      } else {
        metric_is_relaychain_validator_->set(true);
        keypair_ = std::move(keypair->first);
        const auto &authority_index = keypair->second;
        if (lottery_->getEpoch() != current_epoch_) {
          changeLotteryEpoch(current_epoch_, authority_index, babe_config);
        }

        auto slot_leadership = lottery_->getSlotLeadership(current_slot_);

        if (slot_leadership.has_value()) {
          const auto &vrf_result = slot_leadership.value();
          SL_DEBUG(log_,
                   "Babe author {} is primary slot-leader "
                   "(vrfOutput: {}, proof: {})",
                   keypair_->public_key,
                   common::Buffer(vrf_result.output),
                   common::Buffer(vrf_result.proof));

          processSlotLeadership(SlotType::Primary,
                                slot_timestamp,
                                std::cref(vrf_result),
                                authority_index);
        } else if (babe_config.allowed_slots
                       == primitives::AllowedSlots::PrimaryAndSecondaryPlain
                   or babe_config.allowed_slots
                          == primitives::AllowedSlots::PrimaryAndSecondaryVRF) {
          auto expected_author =
              lottery_->secondarySlotAuthor(current_slot_,
                                            babe_config.authorities.size(),
                                            babe_config.randomness);

          if (expected_author.has_value()
              and authority_index == expected_author.value()) {
            if (babe_config.allowed_slots
                == primitives::AllowedSlots::PrimaryAndSecondaryVRF) {
              auto vrf = lottery_->slotVrfSignature(current_slot_);
              SL_DEBUG(log_,
                       "Babe author {} is secondary slot-leader "
                       "(vrfOutput: {}, proof: {})",
                       keypair_->public_key,
                       common::Buffer(vrf.output),
                       common::Buffer(vrf.proof));

              processSlotLeadership(SlotType::SecondaryVRF,
                                    slot_timestamp,
                                    std::cref(vrf),
                                    authority_index);
            } else {  // plain secondary slots mode
              SL_DEBUG(
                  log_,
                  "Babe author {} is block producer in secondary plain slot",
                  keypair_->public_key);

              processSlotLeadership(SlotType::SecondaryPlain,
                                    slot_timestamp,
                                    std::nullopt,
                                    authority_index);
            }
          } else {
            SL_TRACE(log_,
                     "Babe author {} is not block producer in current slot",
                     keypair_->public_key);
          }
        }
      }
    } else {
      SL_ERROR(log_, "Can not get epoch; Skipping slot processing");
    }

    SL_DEBUG(log_,
             "Slot {} in epoch {} has finished",
             current_slot_,
             current_epoch_.epoch_number);

    auto start_time = babe_util_->slotFinishTime(current_slot_);

    SL_DEBUG(log_,
             "Slot {} in epoch {} will start after {}.",
             current_slot_ + 1,
             current_epoch_.epoch_number,
             fmtRemains(*clock_, babe_util_->slotFinishTime(current_slot_)));

    // everything is OK: wait for the end of the slot
    timer_->expiresAt(start_time);
    timer_->asyncWait([this](auto &&ec) {
      if (ec) {
        log_->error("error happened while waiting on the timer: {}", ec);
        return;
      }
      runSlot();
    });
  }

  outcome::result<primitives::PreRuntime> BabeImpl::babePreDigest(
      SlotType slot_type,
      std::optional<std::reference_wrapper<const crypto::VRFOutput>> output,
      primitives::AuthorityIndex authority_index) const {
    BabeBlockHeader babe_header{
        .slot_assignment_type = slot_type,
        .authority_index = authority_index,
        .slot_number = current_slot_,
    };

    if (babe_header.needVRFCheck()) {
      if (not output.has_value()) {
        SL_ERROR(
            log_,
            "VRF proof is required to build block header but was not passed");
        return BabeError::MISSING_PROOF;
      }
      babe_header.vrf_output = output.value();
    }

    auto encoded_header_res = scale::encode(babe_header);
    if (!encoded_header_res) {
      SL_ERROR(log_,
               "cannot encode BabeBlockHeader: {}",
               encoded_header_res.error());
      return encoded_header_res.error();
    }
    common::Buffer encoded_header{encoded_header_res.value()};

    return primitives::PreRuntime{{primitives::kBabeEngineId, encoded_header}};
  }

  outcome::result<primitives::Seal> BabeImpl::sealBlock(
      const primitives::Block &block) const {
    BOOST_ASSERT(keypair_ != nullptr);

    auto pre_seal_encoded_block = scale::encode(block.header).value();

    auto pre_seal_hash = hasher_->blake2b_256(pre_seal_encoded_block);

    Seal seal{};

    if (auto signature = sr25519_provider_->sign(*keypair_, pre_seal_hash);
        signature) {
      seal.signature = signature.value();
    } else {
      SL_ERROR(log_, "Error signing a block seal: {}", signature.error());
      return signature.error();
    }
    auto encoded_seal = common::Buffer(scale::encode(seal).value());
    return primitives::Seal{{primitives::kBabeEngineId, encoded_seal}};
  }

  void BabeImpl::processSlotLeadership(
      SlotType slot_type,
      clock::SystemClock::TimePoint slot_timestamp,
      std::optional<std::reference_wrapper<const crypto::VRFOutput>> output,
      primitives::AuthorityIndex authority_index) {
    auto best_header_res = block_tree_->getBlockHeader(best_block_.hash);
    BOOST_ASSERT_MSG(best_header_res.has_value(),
                     "The best block is always known");
    auto &best_header = best_header_res.value();

    if (backoff(best_header,
                block_tree_->getLastFinalized().number,
                current_slot_)) {
      SL_INFO(log_,
              "Backing off claiming new slot for block authorship: finality is "
              "lagging.");
      return;
    }

    BOOST_ASSERT(keypair_ != nullptr);

    // build a block to be announced
    SL_VERBOSE(log_,
               "Obtained {} slot leadership in slot {} epoch {}",
               slot_type == SlotType::Primary          ? "primary"
               : slot_type == SlotType::SecondaryVRF   ? "secondary-vrf"
               : slot_type == SlotType::SecondaryPlain ? "secondary-plain"
                                                       : "unknown",
               current_slot_,
               current_epoch_.epoch_number);

    SL_INFO(log_, "Babe builds block on top of block {}", best_block_);

    primitives::InherentData inherent_data;
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                   slot_timestamp.time_since_epoch())
                   .count();

    if (auto res = inherent_data.putData<uint64_t>(kTimestampId, now);
        res.has_error()) {
      SL_ERROR(log_, "cannot put an inherent data: {}", res.error());
      return;
    }

    if (auto res = inherent_data.putData(kBabeSlotId, current_slot_);
        res.has_error()) {
      SL_ERROR(log_, "cannot put an inherent data: {}", res.error());
      return;
    }

    ParachainInherentData parachain_inherent_data;

    {
      auto &relay_parent = best_block_.hash;
      parachain_inherent_data.bitfields =
          bitfield_store_->getBitfields(relay_parent);

      parachain_inherent_data.backed_candidates =
          backing_store_->get(relay_parent);
      SL_TRACE(log_,
               "Get backed candidates from store.(count={}, relay_parent={})",
               parachain_inherent_data.backed_candidates.size(),
               relay_parent);

      parachain_inherent_data.parent_header = std::move(best_header);

      {  // Fill disputes
        auto promise_res = std::promise<dispute::MultiDisputeStatementSet>();
        auto res_future = promise_res.get_future();

        dispute_coordinator_->getDisputeForInherentData(
            best_block_,
            [promise_res =
                 std::ref(promise_res)](dispute::MultiDisputeStatementSet res) {
              promise_res.get().set_value(std::move(res));
            });

        if (res_future.valid()) {
          auto res = res_future.get();
          parachain_inherent_data.disputes = std::move(res);
        }
      }
    }

    if (auto res = inherent_data.putData(kParachainId, parachain_inherent_data);
        res.has_error()) {
      SL_ERROR(log_, "cannot put an inherent data: {}", res.error());
      return;
    }

    auto proposal_start = std::chrono::steady_clock::now();
    // calculate babe_pre_digest
    auto babe_pre_digest_res =
        babePreDigest(slot_type, output, authority_index);
    if (not babe_pre_digest_res) {
      SL_ERROR(log_, "cannot propose a block: {}", babe_pre_digest_res.error());
      return;
    }
    const auto &babe_pre_digest = babe_pre_digest_res.value();

    auto propose = [this,
                    inherent_data{std::move(inherent_data)},
                    now,
                    proposal_start,
                    babe_pre_digest{std::move(babe_pre_digest)}] {
      auto changes_tracker =
          std::make_shared<storage::changes_trie::StorageChangesTrackerImpl>();

      // create new block
      auto res = proposer_->propose(best_block_,
                                    babe_util_->slotFinishTime(current_slot_)
                                        - babe_config_repo_->slotDuration() / 3,
                                    inherent_data,
                                    {babe_pre_digest},
                                    changes_tracker);
      if (not res) {
        SL_ERROR(log_, "Cannot propose a block: {}", res.error());
        return;
      }

      processSlotLeadershipProposed(now,
                                    proposal_start,
                                    std::move(changes_tracker),
                                    std::move(res.value()));
    };
    io_context_->post(std::move(propose));
  }

  void BabeImpl::processSlotLeadershipProposed(
      uint64_t now,
      clock::SteadyClock::TimePoint proposal_start,
      std::shared_ptr<storage::changes_trie::StorageChangesTrackerImpl>
          &&changes_tracker,
      primitives::Block &&block) {
    auto duration_ms =
        metric_block_proposal_time.observe(proposal_start).count();
    SL_DEBUG(log_, "Block has been built in {} ms", duration_ms);

    // Ensure block's extrinsics root matches extrinsics in block's body
    BOOST_ASSERT_MSG(
        [&block]() {
          using boost::adaptors::transformed;
          const auto &ext_root_res = storage::trie::calculateOrderedTrieHash(
              storage::trie::StateVersion::V0,
              block.body | transformed([](const auto &ext) {
                return common::Buffer{scale::encode(ext).value()};
              }));
          return ext_root_res.has_value()
             and (ext_root_res.value() == block.header.extrinsics_root);
        }(),
        "Extrinsics root does not match extrinsics in the block");

    // seal the block
    auto seal_res = sealBlock(block);
    if (!seal_res) {
      SL_ERROR(log_, "Failed to seal the block: {}", seal_res.error());
      return;
    }

    // add seal digest item
    block.header.digest.emplace_back(seal_res.value());

    if (clock_->now()
        >= babe_util_->slotFinishTime(current_slot_ + kMaxBlockSlotsOvertime)) {
      SL_WARN(log_,
              "Block was not built in time. "
              "Allowed slots ({}) have passed. "
              "If you are executing in debug mode, consider to rebuild in "
              "release",
              kMaxBlockSlotsOvertime);
      return;
    }

    const auto block_hash =
        hasher_->blake2b_256(scale::encode(block.header).value());
    const primitives::BlockInfo block_info(block.header.number, block_hash);

    auto last_finalized_block = block_tree_->getLastFinalized();
    auto previous_best_block_res =
        block_tree_->getBestContaining(last_finalized_block.hash, std::nullopt);
    BOOST_ASSERT(previous_best_block_res.has_value());
    const auto &previous_best_block = previous_best_block_res.value();

    // add block to the block tree
    if (auto add_res = block_tree_->addBlock(block); not add_res) {
      SL_ERROR(log_, "Could not add block {}: {}", block_info, add_res.error());
      auto removal_res = block_tree_->removeLeaf(block_hash);
      if (removal_res.has_error()
          and removal_res
                  != outcome::failure(
                      blockchain::BlockTreeError::BLOCK_IS_NOT_LEAF)) {
        SL_WARN(log_,
                "Rolling back of block {} is failed: {}",
                block_info,
                removal_res.error());
      }
      return;
    }

    changes_tracker->onBlockAdded(
        block_hash, storage_sub_engine_, chain_events_engine_);

    telemetry_->notifyBlockImported(block_info, telemetry::BlockOrigin::kOwn);

    // observe digest of block
    // (must be done strictly after block will be added)
    auto digest_tracking_res = digest_tracker_->onDigest(
        {.block_info = block_info, .header = block.header},
        block.header.digest);

    if (digest_tracking_res.has_error()) {
      SL_WARN(log_,
              "Error while tracking digest of block {}: {}",
              block_info,
              digest_tracking_res.error());
      return;
    }

    // finally, broadcast the sealed block
    block_announce_transmitter_->blockAnnounce(network::BlockAnnounce{
        block.header,
        block_info == block_tree_->bestLeaf() ? network::BlockState::Best
                                              : network::BlockState::Normal,
        common::Buffer{},
    });
    SL_DEBUG(
        log_,
        "Announced block number {} in slot {} (epoch {}) with timestamp {}",
        block.header.number,
        current_slot_,
        current_epoch_.epoch_number,
        now);

    last_finalized_block = block_tree_->getLastFinalized();
    auto current_best_block_res =
        block_tree_->getBestContaining(last_finalized_block.hash, std::nullopt);
    BOOST_ASSERT(current_best_block_res.has_value());
    const auto &current_best_block = current_best_block_res.value();

    // Create new offchain worker for block if it is best only
    if (current_best_block.number > previous_best_block.number) {
      auto ocw_res = offchain_worker_api_->offchain_worker(
          block.header.parent_hash, block.header);
      if (ocw_res.has_failure()) {
        log_->error("Can't spawn offchain worker for block {}: {}",
                    block_info,
                    ocw_res.error());
      }
    }
  }

  void BabeImpl::changeLotteryEpoch(
      const EpochDescriptor &epoch,
      primitives::AuthorityIndex authority_index,
      const primitives::BabeConfiguration &babe_config) const {
    BOOST_ASSERT(keypair_ != nullptr);

    auto threshold = calculateThreshold(
        babe_config.leadership_rate, babe_config.authorities, authority_index);

    lottery_->changeEpoch(epoch, babe_config.randomness, threshold, *keypair_);
  }
}  // namespace kagome::consensus::babe
