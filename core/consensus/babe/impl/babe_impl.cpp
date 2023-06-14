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
#include "network/block_announce_transmitter.hpp"
#include "network/helpers/peer_id_formatter.hpp"
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

namespace {
  constexpr const char *kBlockProposalTime =
      "kagome_proposer_block_constructed";
}

using namespace std::literals::chrono_literals;

namespace kagome::consensus::babe {
  using ParachainInherentData = parachain::ParachainInherentData;
  using SyncMethod = application::AppConfiguration::SyncMethod;

  BabeImpl::BabeImpl(
      const application::AppConfiguration &app_config,
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<BabeLottery> lottery,
      std::shared_ptr<BabeConfigRepository> babe_config_repo,
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
      : app_config_(app_config),
        app_state_manager_(app_state_manager),
        lottery_{std::move(lottery)},
        babe_config_repo_{std::move(babe_config_repo)},
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
    metrics_registry_->registerHistogramFamily(
        kBlockProposalTime, "Time taken to construct new block");
    metric_block_proposal_time_ = metrics_registry_->registerHistogramMetric(
        kBlockProposalTime,
        {0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1, 2.5, 5, 10});

    app_state_manager_->takeControl(*this);
  }

  bool BabeImpl::prepare() {
    auto res = getInitialEpochDescriptor();
    if (res.has_error()) {
      SL_CRITICAL(log_, "Can't get initial epoch descriptor: {}", res.error());
      return false;
    }

    best_block_ = block_tree_->bestLeaf();

    // Check if best block has state for usual sync method
    if (app_config_.syncMethod() == SyncMethod::Full) {
      auto best_block_header_res =
          block_tree_->getBlockHeader(best_block_.hash);
      if (best_block_header_res.has_error()) {
        SL_CRITICAL(log_,
                    "Can't get header of best block ({}): {}",
                    best_block_,
                    best_block_header_res.error());
        return false;
      }
      const auto &best_block_header = best_block_header_res.value();

      const auto &state_root = best_block_header.state_root;

      // Check if target block has state
      if (auto res = trie_storage_->getEphemeralBatchAt(state_root);
          res.has_error()) {
        SL_WARN(log_, "Can't get state of best block: {}", res.error());
        SL_CRITICAL(log_,
                    "Try restart at least once with `--sync Fast' CLI arg");
        return false;
      }
    }

    current_epoch_ = res.value();

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
    best_block_ = block_tree_->bestLeaf();

    SL_DEBUG(log_, "Babe is starting with syncing from block {}", best_block_);

    SL_DEBUG(log_,
             "Starting in epoch {} and slot {}",
             current_epoch_.epoch_number,
             current_epoch_.start_slot);

    auto babe_config = babe_config_repo_->config({.block_info = best_block_},
                                                 current_epoch_.epoch_number);
    if (not babe_config.has_value()) {
      SL_CRITICAL(
          log_,
          "Can't obtain digest of epoch {} from block tree for block {}",
          current_epoch_.epoch_number,
          best_block_);
      return false;
    }
    const auto &authorities = babe_config->get().authorities;
    if (authorities.size() == 1 && session_keys_->getBabeKeyPair(authorities)) {
      SL_INFO(log_, "Starting single validating node.");
      onSynchronized();
      return true;
    }

    switch (app_config_.syncMethod()) {
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
    }

    return true;
  }

  outcome::result<EpochDescriptor> BabeImpl::getInitialEpochDescriptor() {
    auto best_block = block_tree_->bestLeaf();

    if (best_block.number == 0) {
      EpochDescriptor epoch_descriptor{
          .epoch_number = 0,
          .start_slot =
              static_cast<BabeSlotNumber>(clock_->now().time_since_epoch()
                                          / babe_config_repo_->slotDuration())
              + 1};
      return outcome::success(epoch_descriptor);
    }

    // Look up slot number of best block
    auto best_block_header_res = block_tree_->getBlockHeader(best_block.hash);
    BOOST_ASSERT_MSG(best_block_header_res.has_value(),
                     "Best block must be known whenever");
    const auto &best_block_header = best_block_header_res.value();
    auto babe_digest_res = getBabeDigests(best_block_header);
    BOOST_ASSERT_MSG(babe_digest_res.has_value(),
                     "Any non genesis block must contain babe digest");
    auto last_slot_number = babe_digest_res.value().second.slot_number;

    EpochDescriptor epoch_descriptor{
        .epoch_number = babe_util_->slotToEpoch(last_slot_number),
        .start_slot =
            last_slot_number - babe_util_->slotInEpoch(last_slot_number)};

    return outcome::success(epoch_descriptor);
  }

  void BabeImpl::adjustEpochDescriptor() {
    auto first_slot_number = babe_util_->syncEpoch([&]() {
      auto hash_res = block_tree_->getBlockHash(primitives::BlockNumber(1));
      if (hash_res.has_error()) {
        SL_TRACE(log_,
                 "First block slot is {}: no first block (at adjusting)",
                 babe_util_->getCurrentSlot());
        return std::tuple(babe_util_->getCurrentSlot(), false);
      }

      auto first_block_header_res =
          block_tree_->getBlockHeader(hash_res.value());
      if (first_block_header_res.has_error()) {
        SL_CRITICAL(log_,
                    "Database is not consistent: "
                    "Not found block header for existing num-to-hash record");
        throw std::runtime_error(
            "Not found block header for existing num-to-hash record");
      }

      const auto &first_block_header = first_block_header_res.value();
      auto babe_digest_res = getBabeDigests(first_block_header);
      BOOST_ASSERT_MSG(babe_digest_res.has_value(),
                       "Any non genesis block must contain babe digest");
      auto first_slot_number = babe_digest_res.value().second.slot_number;

      auto is_first_block_finalized =
          block_tree_->getLastFinalized().number > 0;

      SL_TRACE(
          log_,
          "First block slot is {}: by {}finalized first block (at adjusting)",
          first_slot_number,
          is_first_block_finalized ? "" : "non-");
      return std::tuple(first_slot_number, is_first_block_finalized);
    });

    auto current_epoch_start_slot =
        first_slot_number
        + current_epoch_.epoch_number * babe_config_repo_->epochLength();

    if (current_epoch_.start_slot != current_epoch_start_slot) {
      SL_WARN(log_,
              "Start-slot of current epoch {} has updated from {} to {}",
              current_epoch_.epoch_number,
              current_epoch_.start_slot,
              current_epoch_start_slot);

      current_epoch_.start_slot = current_epoch_start_slot;
    }
  }

  void BabeImpl::runEpoch(EpochDescriptor epoch) {
    bool already_active = false;
    if (not active_.compare_exchange_strong(already_active, true)) {
      return;
    }

    adjustEpochDescriptor();

    SL_DEBUG(log_,
             "Starting an epoch {}. Secondary slots allowed={}",
             epoch.epoch_number,
             babe_config_repo_
                 ->config({.block_info = best_block_}, epoch.epoch_number)
                 ->get()
                 .isSecondarySlotsAllowed());
    current_epoch_ = epoch;
    current_slot_ = current_epoch_.start_slot;

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
        onSynchronized();
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

            // Just synced
            if (self->current_state_ == Babe::State::CATCHING_UP) {
              SL_INFO(self->log_, "Catching up is finished on block {}", block);
              self->current_state_ = Babe::State::SYNCHRONIZED;
              self->was_synchronized_ = true;
              self->telemetry_->notifyWasSynchronized();
              self->babe_status_observable_->notify(
                  primitives::events::BabeStateEventType::kSyncState,
                  self->current_state_);
            }

            // Synced
            if (self->current_state_ == Babe::State::SYNCHRONIZED) {
              self->onSynchronized();
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
    if (app_config_.syncMethod() != SyncMethod::Warp) {
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

    if (app_config_.syncMethod() == SyncMethod::FastWithoutState) {
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

    synchronizer_->syncBabeDigest(
        peer_id,
        block_at_state,
        [=, weak{weak_from_this()}](outcome::result<void> res) {
          auto self = weak.lock();
          if (not self) {
            return;
          }
          if (not res) {
            SL_WARN(self->log_,
                    "Syncing of babe digests with {} on block {} is failed: {}",
                    peer_id,
                    block_at_state,
                    res.error());
            return;
          }
          synchronizer_->syncState(
              peer_id,
              block_at_state,
              [wp = weak_from_this(), block_at_state, peer_id](
                  auto res) mutable {
                if (auto self = wp.lock()) {
                  if (res.has_error()) {
                    SL_WARN(
                        self->log_,
                        "Syncing of state with {} on block {} is failed: {}",
                        peer_id,
                        block_at_state,
                        res.error());
                    return;
                  }

                  self->adjustEpochDescriptor();
                  self->babe_config_repo_->readFromState(block_at_state);
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
        });
  }

  void BabeImpl::onSynchronized() {
    current_state_ = State::SYNCHRONIZED;
    was_synchronized_ = true;
    telemetry_->notifyWasSynchronized();
    babe_status_observable_->notify(
        primitives::events::BabeStateEventType::kSyncState, current_state_);

    if (not active_) {
      best_block_ = block_tree_->bestLeaf();
      SL_DEBUG(log_, "Babe is synchronized on block {}", best_block_);
      runEpoch(current_epoch_);
    }
  }

  bool BabeImpl::wasSynchronized() const {
    return was_synchronized_;
  }

  void BabeImpl::runSlot() {
    bool rewind_slots;  // NOLINT
    auto slot = current_slot_;

    clock::SystemClock::TimePoint now;
    do {
      // check that we are really in the middle of the slot, as expected; we
      // can cooperate with a relatively little (kMaxLatency) latency, as our
      // node will be able to retrieve
      now = clock_->now();

      auto finish_time = babe_util_->slotFinishTime(current_slot_);

      rewind_slots = now > finish_time
                 and (now - finish_time) > babe_config_repo_->slotDuration();

      if (rewind_slots) {
        // we are too far behind; after skipping some slots (but not epochs)
        // control will be returned to this method

        ++current_slot_;

        if (current_epoch_.epoch_number
            != babe_util_->slotToEpoch(current_slot_)) {
          startNextEpoch();
        } else {
          adjustEpochDescriptor();
        }
      } else if (slot < current_slot_) {
        SL_VERBOSE(log_, "Slots {}..{} was skipped", slot, current_slot_ - 1);
      }
    } while (rewind_slots);

    SL_VERBOSE(log_,
               "Starting a slot {} in epoch {} (remains {:.2f} sec.)",
               current_slot_,
               current_epoch_.epoch_number,
               std::chrono::duration_cast<std::chrono::milliseconds>(
                   babe_util_->remainToFinishOfSlot(current_slot_))
                       .count()
                   / 1000.);

    processSlot(now);
  }

  void BabeImpl::processSlot(clock::SystemClock::TimePoint slot_timestamp) {
    best_block_ = block_tree_->bestLeaf();

    // Resolve slot collisions: if best block slot greater than current slot,
    // that select his ancestor as best
    for (;;) {
      const auto &hash = best_block_.hash;
      const auto header_res = block_tree_->getBlockHeader(hash);
      BOOST_ASSERT(header_res.has_value());
      const auto &header = header_res.value();
      const auto babe_digests_res = getBabeDigests(header);
      if (babe_digests_res.has_value()) {
        const auto &babe_digests = babe_digests_res.value();
        auto best_block_slot = babe_digests.second.slot_number;
        if (current_slot_ > best_block_slot) {  // Condition met
          break;
        }
        SL_DEBUG(log_, "Detected collision in slot {}", current_slot_);
        // Shift to parent block and check again
        best_block_ =
            primitives::BlockInfo(header.number - 1, header.parent_hash);
        continue;
      }
      if (best_block_.number == 0) {
        // Only genesis block header might not have a babe digest
        break;
      }
      BOOST_ASSERT(babe_digests_res.has_value());
    }

    auto babe_config_opt = babe_config_repo_->config(
        {.block_info = best_block_}, current_epoch_.epoch_number);
    if (babe_config_opt) {
      auto &babe_config = babe_config_opt.value().get();
      auto keypair = session_keys_->getBabeKeyPair(babe_config.authorities);
      if (not keypair) {
        SL_ERROR(log_,
                 "Authority not known, skipping slot processing. "
                 "Probably authority list has changed.");
      } else {
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

    ++current_slot_;

    if (current_epoch_.epoch_number != babe_util_->slotToEpoch(current_slot_)) {
      startNextEpoch();
    } else {
      adjustEpochDescriptor();
    }

    auto start_time = babe_util_->slotStartTime(current_slot_);

    SL_DEBUG(log_,
             "Slot {} in epoch {} will start after {:.2f} sec.",
             current_slot_,
             current_epoch_.epoch_number,
             std::chrono::duration_cast<std::chrono::milliseconds>(
                 babe_util_->remainToStartOfSlot(current_slot_))
                     .count()
                 / 1000.);

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

    auto proposal_start = std::chrono::high_resolution_clock::now();
    // calculate babe_pre_digest
    auto babe_pre_digest_res =
        babePreDigest(slot_type, output, authority_index);
    if (not babe_pre_digest_res) {
      SL_ERROR(log_, "cannot propose a block: {}", babe_pre_digest_res.error());
      return;
    }
    const auto &babe_pre_digest = babe_pre_digest_res.value();

    auto changes_tracker =
        std::make_shared<storage::changes_trie::StorageChangesTrackerImpl>();

    // create new block
    auto pre_seal_block_res =
        proposer_->propose(best_block_,
                           babe_util_->slotFinishTime(current_slot_)
                               - babe_config_repo_->slotDuration() / 3,
                           inherent_data,
                           {babe_pre_digest},
                           changes_tracker);
    if (!pre_seal_block_res) {
      SL_ERROR(log_, "Cannot propose a block: {}", pre_seal_block_res.error());
      return;
    }

    auto proposal_end = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                           proposal_end - proposal_start)
                           .count();
    SL_DEBUG(log_, "Block has been built in {} ms", duration_ms);

    metric_block_proposal_time_->observe(static_cast<double>(duration_ms)
                                         / 1000);

    auto block = pre_seal_block_res.value();

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
             and (ext_root_res.value()
                  == common::Buffer(block.header.extrinsics_root));
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

    if (babe_util_->remainToFinishOfSlot(current_slot_ + kMaxBlockSlotsOvertime)
            .count()
        == 0) {
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
        babe_util_->slotToEpoch(current_slot_),
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

  void BabeImpl::startNextEpoch() {
    SL_DEBUG(log_,
             "Epoch {} has finished. Start epoch {}",
             current_epoch_.epoch_number,
             current_epoch_.epoch_number + 1);

    ++current_epoch_.epoch_number;
    current_epoch_.start_slot = current_slot_;

    babe_util_->syncEpoch([&]() {
      auto hash_res = block_tree_->getBlockHash(primitives::BlockNumber(1));
      if (hash_res.has_error()) {
        SL_TRACE(log_,
                 "First block slot is {}: no first block (at start next epoch)",
                 babe_util_->getCurrentSlot());
        return std::tuple(babe_util_->getCurrentSlot(), false);
      }

      auto first_block_header_res =
          block_tree_->getBlockHeader(hash_res.value());
      if (first_block_header_res.has_error()) {
        SL_CRITICAL(log_,
                    "Database is not consistent: "
                    "Not found block header for existing num-to-hash record");
        throw std::runtime_error(
            "Not found block header for existing num-to-hash record");
      }

      const auto &first_block_header = first_block_header_res.value();
      auto babe_digest_res = getBabeDigests(first_block_header);
      BOOST_ASSERT_MSG(babe_digest_res.has_value(),
                       "Any non genesis block must contain babe digest");
      auto first_slot_number = babe_digest_res.value().second.slot_number;

      auto is_first_block_finalized =
          block_tree_->getLastFinalized().number > 0;

      SL_WARN(log_,
              "First block slot is {}: "
              "by {}finalized first block (at start next epoch)",
              first_slot_number,
              is_first_block_finalized ? "" : "non-");
      return std::tuple(first_slot_number, is_first_block_finalized);
    });
  }

}  // namespace kagome::consensus::babe
