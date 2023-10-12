/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/timeline/impl/timeline_impl.hpp"

#include "application/app_configuration.hpp"
#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "clock/impl/clock_impl.hpp"
#include "consensus/consensus_selector.hpp"
#include "consensus/grandpa/justification_observer.hpp"
#include "consensus/timeline/consistency_keeper.hpp"
#include "consensus/timeline/impl/block_production_error.hpp"
#include "consensus/timeline/slots_util.hpp"
#include "network/block_announce_transmitter.hpp"
#include "network/synchronizer.hpp"
#include "network/warp/protocol.hpp"
#include "network/warp/sync.hpp"
#include "network/warp/types.hpp"
#include "runtime/runtime_api/core.hpp"
#include "storage/trie/trie_storage.hpp"

#include <libp2p/basic/scheduler.hpp>

namespace {
  constexpr const char *kIsMajorSyncing = "kagome_sub_libp2p_is_major_syncing";
}

namespace kagome::consensus {

  using application::SyncMethod;

  TimelineImpl::TimelineImpl(
      const application::AppConfiguration &app_config,
      std::shared_ptr<application::AppStateManager> app_state_manager,
      const clock::SystemClock &clock,
      std::shared_ptr<SlotsUtil> slots_util,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<ConsensusSelector> consensus_selector,
      std::shared_ptr<storage::trie::TrieStorage> trie_storage,
      std::shared_ptr<network::Synchronizer> synchronizer,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<network::BlockAnnounceTransmitter>
          block_announce_transmitter,
      std::shared_ptr<network::WarpSync> warp_sync,
      LazySPtr<network::WarpProtocol> warp_protocol,
      std::shared_ptr<consensus::grandpa::JustificationObserver>
          justification_observer,
      std::shared_ptr<ConsistencyKeeper> consistency_keeper,
      std::shared_ptr<libp2p::basic::Scheduler> scheduler,
      primitives::events::ChainSubscriptionEnginePtr chain_sub_engine,
      primitives::events::BabeStateSubscriptionEnginePtr state_sub_engine,
      std::shared_ptr<runtime::Core> core_api)
      : log_(log::createLogger("Timeline", "timeline")),
        app_state_manager_(std::move(app_state_manager)),
        clock_(clock),
        slots_util_(std::move(slots_util)),
        block_tree_(std::move(block_tree)),
        consensus_selector_(std::move(consensus_selector)),
        trie_storage_(std::move(trie_storage)),
        synchronizer_(std::move(synchronizer)),
        hasher_(std::move(hasher)),
        block_announce_transmitter_(std::move(block_announce_transmitter)),
        warp_sync_(std::move(warp_sync)),
        warp_protocol_(std::move(warp_protocol)),
        justification_observer_(std::move(justification_observer)),
        consistency_keeper_(std::move(consistency_keeper)),
        scheduler_(std::move(scheduler)),
        chain_sub_engine_(std::move(chain_sub_engine)),
        chain_sub_{chain_sub_engine_},
        state_sub_engine_(std::move(state_sub_engine)),
        core_api_(std::move(core_api)),
        sync_method_(app_config.syncMethod()),
        telemetry_{telemetry::createTelemetryService()} {
    BOOST_ASSERT(app_state_manager_);
    BOOST_ASSERT(slots_util_);
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(consensus_selector_);
    BOOST_ASSERT(trie_storage_);
    BOOST_ASSERT(synchronizer_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(block_announce_transmitter_);
    // BOOST_ASSERT(warp_sync_);
    BOOST_ASSERT(justification_observer_);
    BOOST_ASSERT(consistency_keeper_);
    BOOST_ASSERT(scheduler_);
    BOOST_ASSERT(chain_sub_engine_);
    BOOST_ASSERT(state_sub_engine_);
    BOOST_ASSERT(core_api_);

    // Register metrics
    metrics_registry_->registerGaugeFamily(
        kIsMajorSyncing, "Whether the node is performing a major sync or not.");
    metric_is_major_syncing_ =
        metrics_registry_->registerGaugeMetric(kIsMajorSyncing);
    metric_is_major_syncing_->set(!was_synchronized_);

    app_state_manager_->takeControl(*this);
  }

  namespace {
    std::tuple<std::chrono::microseconds,
               std::chrono::microseconds,
               std::chrono::microseconds>
    estimateSyncDuration(size_t lag_slots, Duration slot_duration) {
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

  bool TimelineImpl::prepare() {
    updateSlot(clock_.now());

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

    auto consensus = consensus_selector_->getProductionConsensus(best_block_);

    // Calculate lag our best block by slots
    SlotNumber lag_slots = 0;
    if (auto slot_res = consensus->getSlot(best_block_header)) {
      lag_slots = slots_util_->timeToSlot(clock_.now()) - slot_res.value();
    }

    auto &&[warp_sync_duration, fast_sync_duration, full_sync_duration] =
        estimateSyncDuration(lag_slots, slots_util_->slotDuration());

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

    chain_sub_.onFinalize([weak{weak_from_this()}](
                              const primitives::BlockHeader &block) {
      if (auto self = weak.lock()) {
        if (self->current_state_ != SyncState::HEADERS_LOADING
            and self->current_state_ != SyncState::STATE_LOADING) {
          auto version_res = self->core_api_->version(block.hash());
          if (version_res.has_value()) {
            auto &version = version_res.value();
            if (not self->actual_runtime_version_.has_value()
                or self->actual_runtime_version_ != version) {
              self->actual_runtime_version_ = version;
              self->chain_sub_engine_->notify(
                  primitives::events::ChainEventType::kFinalizedRuntimeVersion,
                  version);
            }
          }
        }
      }
    });

    return true;
  }

  bool TimelineImpl::start() {
    updateSlot(clock_.now());

    SL_DEBUG(log_, "Babe is starting with syncing from block {}", best_block_);

    SL_DEBUG(log_,
             "Starting in epoch {} and slot {}",
             current_epoch_,
             current_slot_);

    if (sync_method_ != SyncMethod::Warp) {
      auto consensus = consensus_selector_->getProductionConsensus(best_block_);

      auto validator_status =
          consensus->getValidatorStatus(best_block_, current_epoch_);

      if (validator_status == ValidatorStatus::SingleValidator) {
        SL_INFO(log_, "Starting single validating node.");
        onSynchronized();
        return true;
      }
    }

    switch (sync_method_) {
      case SyncMethod::Full:
        current_state_ = SyncState::WAIT_REMOTE_STATUS;
        break;

      case SyncMethod::Fast:
      case SyncMethod::Warp:
      case SyncMethod::FastWithoutState: {
        current_state_ = SyncState::HEADERS_LOADING;
        state_sub_engine_->notify(
            primitives::events::SyncStateEventType::kSyncState, current_state_);
      } break;

      case SyncMethod::Auto:
        UNREACHABLE;  // It must be rewritten in prepare stage
    }

    return true;
  }

  SyncState TimelineImpl::getCurrentState() const {
    return current_state_;
  }

  bool TimelineImpl::wasSynchronized() const {
    return was_synchronized_;
  }

  void TimelineImpl::onBlockAnnounceHandshake(
      const libp2p::peer::PeerId &peer_id,
      const network::BlockAnnounceHandshake &handshake) {
    // If state is loading, just to ping of loading
    if (current_state_ == SyncState::STATE_LOADING) {
      startStateSyncing(peer_id);
      return;
    }

    if (warpSync(peer_id, handshake.best_block.number)) {
      return;
    }

    auto current_best_block = block_tree_->bestBlock();

    if (current_best_block == handshake.best_block) {
      if (current_state_ == SyncState::HEADERS_LOADING) {
        current_state_ = SyncState::HEADERS_LOADED;
        state_sub_engine_->notify(
            primitives::events::SyncStateEventType::kSyncState, current_state_);
        startStateSyncing(peer_id);
      } else if (current_state_ == SyncState::CATCHING_UP
                 or current_state_ == SyncState::WAIT_REMOTE_STATUS) {
        onCaughtUp(current_best_block);
      }
      return;
    }

    // Remote peer is lagged
    const auto &last_finalized_block = block_tree_->getLastFinalized();
    if (handshake.best_block.number <= last_finalized_block.number) {
      return;
    }

    startCatchUp(peer_id, handshake.best_block);
  }

  void TimelineImpl::onBlockAnnounce(const libp2p::peer::PeerId &peer_id,
                                     const network::BlockAnnounce &announce) {
    // If state is loading, just to ping of loading
    if (current_state_ == SyncState::STATE_LOADING) {
      startStateSyncing(peer_id);
      return;
    }

    if (warpSync(peer_id, announce.header.number)) {
      return;
    }

    auto current_best_block = block_tree_->bestBlock();

    // Skip obsoleted announce
    if (announce.header.number < current_best_block.number) {
      return;
    }

    // Start catching up if gap recognized
    if (current_state_ == SyncState::SYNCHRONIZED
        or current_state_ == SyncState::HEADERS_LOADED) {
      if (announce.header.number > current_best_block.number + 1) {
        startCatchUp(peer_id, announce.header.blockInfo());
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
            if (self->current_state_ == SyncState::HEADERS_LOADING) {
              self->current_state_ = SyncState::HEADERS_LOADED;
              self->state_sub_engine_->notify(
                  primitives::events::SyncStateEventType::kSyncState,
                  self->current_state_);
              self->startStateSyncing(peer_id);
              return;
            }

            // Caught up some block, possible block of current slot
            if (self->current_state_ == SyncState::CATCHING_UP
                or self->current_state_ == SyncState::WAIT_REMOTE_STATUS) {
              self->onCaughtUp(block);
            }

            // Synced
            if (self->current_state_ == SyncState::SYNCHRONIZED) {
              // Set actual block status
              announce.state = block == self->block_tree_->bestBlock()
                                 ? network::BlockState::Best
                                 : network::BlockState::Normal;
              // Propagate announce
              self->block_announce_transmitter_->blockAnnounce(
                  std::move(announce));
              return;
            }
          }
        });
  };

  bool TimelineImpl::updateSlot(TimePoint now) {
    best_block_ = block_tree_->bestBlock();

    auto prev_slot = current_slot_;
    current_slot_ = slots_util_->timeToSlot(now);
    if (current_slot_ == prev_slot) {
      SL_TRACE(log_,
               "Slot was not updated, it is still the same: {}",
               current_slot_);
      return false;
    }

    auto epoch_res = slots_util_->slotToEpoch(best_block_, current_slot_);
    if (not epoch_res) {
      SL_ERROR(log_,
               "Slot update has failed; can't get epoch: {}",
               epoch_res.error());
      return false;
    }
    current_epoch_ = epoch_res.value();

    SL_DEBUG(log_,
             "Slot was updated to {}, epoch is {}, best block is {}",
             current_slot_,
             current_epoch_,
             best_block_);
    return true;
  }

  bool TimelineImpl::warpSync(const libp2p::peer::PeerId &peer_id,
                              primitives::BlockNumber block_number) {
    if (current_state_ != SyncState::HEADERS_LOADING) {
      return false;
    }
    if (sync_method_ != SyncMethod::Warp) {
      return false;
    }
    auto target = warp_sync_->request();
    if (not target) {
      current_state_ = SyncState::HEADERS_LOADED;
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

  void TimelineImpl::startCatchUp(const libp2p::peer::PeerId &peer_id,
                                  const primitives::BlockInfo &target_block) {
    BOOST_ASSERT(current_state_ != SyncState::STATE_LOADING);

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
      if (current_state_ == SyncState::HEADERS_LOADED) {
        current_state_ = SyncState::HEADERS_LOADING;
        state_sub_engine_->notify(
            primitives::events::SyncStateEventType::kSyncState, current_state_);
      } else if (current_state_ == SyncState::WAIT_BLOCK_ANNOUNCE
                 or current_state_ == SyncState::WAIT_REMOTE_STATUS
                 or current_state_ == SyncState::SYNCHRONIZED) {
        current_state_ = SyncState::CATCHING_UP;
        state_sub_engine_->notify(
            primitives::events::SyncStateEventType::kSyncState, current_state_);
      }
    }
  }

  void TimelineImpl::onCaughtUp(const primitives::BlockInfo &block) {
    SL_INFO(log_, "Caught up block {}", block);

    if (not was_synchronized_) {
      auto header_opt = block_tree_->getBlockHeader(block.hash);
      BOOST_ASSERT_MSG(header_opt.has_value(), "Just added block; deq");
      const auto &header = header_opt.value();

      [[likely]] if (header.number != 0) {
        auto consensus = consensus_selector_->getProductionConsensus(block);

        auto slot_res = consensus->getSlot(header);
        if (not slot_res.has_value()) {
          return;
        }
        if (slots_util_->timeToSlot(clock_.now()) > slot_res.value() + 1) {
          current_state_ = SyncState::WAIT_REMOTE_STATUS;
          state_sub_engine_->notify(
              primitives::events::SyncStateEventType::kSyncState,
              current_state_);
          return;
        }
      }
    }

    onSynchronized();
  }

  void TimelineImpl::startStateSyncing(const libp2p::peer::PeerId &peer_id) {
    BOOST_ASSERT(current_state_ == SyncState::HEADERS_LOADED
                 or current_state_ == SyncState::STATE_LOADING);
    if (current_state_ != SyncState::HEADERS_LOADED
        and current_state_ != SyncState::STATE_LOADING) {
      SL_WARN(log_, "Syncing of state can not be start: Bad state of timeline");
      return;
    }

    current_state_ = SyncState::STATE_LOADING;
    state_sub_engine_->notify(
        primitives::events::SyncStateEventType::kSyncState, current_state_);

    auto best_block =
        block_tree_->getBlockHeader(block_tree_->bestBlock().hash).value();
    if (trie_storage_->getEphemeralBatchAt(best_block.state_root)) {
      current_state_ = SyncState::CATCHING_UP;
      return;
    }

    if (sync_method_ == SyncMethod::FastWithoutState) {
      if (app_state_manager_->state()
          != application::AppStateManager::State::ShuttingDown) {
        SL_INFO(log_,
                "Stateless fast sync is finished on block {}; "
                "Application is stopping",
                block_tree_->bestBlock());
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

    block_tree_->removeUnfinalized();

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
            self->current_state_ = SyncState::CATCHING_UP;
            self->state_sub_engine_->notify(
                primitives::events::SyncStateEventType::kSyncState,
                self->current_state_);
          }
        });
  }

  void TimelineImpl::onSynchronized() {
    if (not was_synchronized_) {
      was_synchronized_ = true;

      telemetry_->notifyWasSynchronized();
    }
    metric_is_major_syncing_->set(!was_synchronized_);
    current_state_ = SyncState::SYNCHRONIZED;

    state_sub_engine_->notify(
        primitives::events::SyncStateEventType::kSyncState, current_state_);

    if (not active_) {
      best_block_ = block_tree_->bestBlock();
      SL_DEBUG(log_, "Node is synchronized on block {}", best_block_);
      runEpoch();
    }
  }

  void TimelineImpl::runEpoch() {
    bool already_active = false;
    if (not active_.compare_exchange_strong(already_active, true)) {
      return;
    }
    runSlot();
  }

  void TimelineImpl::runSlot() {
    SL_TRACE(log_, "Try to run slot");
    auto now = clock_.now();

    auto slot_has_updated = updateSlot(now);

    auto remains_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::max(slots_util_->slotFinishTime(current_slot_) - now, {}));

    if (not slot_has_updated) {
      SL_DEBUG(log_,
               "Not updated. Waiting for end of slot {} (remains {:.2f} sec)",
               current_slot_,
               remains_ms.count() / 1000.);

      scheduler_->schedule(
          [wp = weak_from_this()] {
            if (auto self = wp.lock()) {
              self->runSlot();
            }
          },
          remains_ms.count() ? remains_ms : std::chrono::milliseconds(30));
      return;
    }

    SL_VERBOSE(log_,
               "Running a slot {} in epoch {} (remains {:.2f} sec)",
               current_slot_,
               current_epoch_,
               remains_ms.count() / 1000.);

    processSlot(now);
  }

  void TimelineImpl::processSlot(TimePoint timestamp) {
    // Check cases based on current best block slot
    [[likely]] if (best_block_.number != 0) {
      auto parent_header =
          block_tree_->getBlockHeader(best_block_.hash).value();

      auto consensus = consensus_selector_->getProductionConsensus(best_block_);

      auto parent_slot = consensus->getSlot(parent_header).value();
      if (parent_slot > current_slot_) {
        SL_WARN(log_,
                "Best block {} of future slot, but current is slot {}; "
                "Seems clock is lagging",
                best_block_,
                current_slot_);
        return;
      }
      if (parent_slot == current_slot_) {
        // fork or wait next slot
        SL_INFO(log_,
                "Concurrent best block {}, current slot {}, could be a fork",
                best_block_,
                current_slot_);
        best_block_ = *parent_header.parentInfo();
      }
    }

    static const auto &block_production_error_category =
        make_error_code(BlockProductionError{}).category();

    /// Try to run block production here
    auto consensus = consensus_selector_->getProductionConsensus(best_block_);
    auto res = consensus->processSlot(current_slot_, best_block_);
    if (res.has_value()) {
      SL_DEBUG(log_,
               "Slot {} in epoch {} has processed",
               current_slot_,
               current_epoch_);
    } else if (res.as_failure().error().category()
               == block_production_error_category) {
      SL_DEBUG(log_,
               "Processing of slot {} was skipped: {}",
               current_slot_,
               res.error());
    } else {
      SL_ERROR(log_,
               "Processing of slot {} has failed: {}",
               current_slot_,
               res.error());
    }

    auto remains_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::max(
            slots_util_->slotFinishTime(current_slot_) - clock_.now(), {}));

    if (remains_ms > std::chrono::milliseconds(0)) {
      SL_DEBUG(log_,
               "Waiting for end of slot {} (remains {:.2f} sec.)",
               current_slot_,
               remains_ms.count() / 1000.);
    }

    // everything is OK: wait for the end of the slot
    scheduler_->schedule(
        [wp = weak_from_this()] {
          if (auto self = wp.lock()) {
            self->runSlot();
          }
        },
        remains_ms);
  }

}  // namespace kagome::consensus
