/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/babe_impl.hpp"

#include <boost/assert.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include "application/app_configuration.hpp"
#include "blockchain/block_storage_error.hpp"
#include "blockchain/block_tree_error.hpp"
#include "common/buffer.hpp"
#include "consensus/babe/babe_config_repository.hpp"
#include "consensus/babe/babe_error.hpp"
#include "consensus/babe/consistency_keeper.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "consensus/babe/impl/parachains_inherent_data.hpp"
#include "consensus/babe/impl/threshold_util.hpp"
#include "consensus/babe/types/babe_block_header.hpp"
#include "consensus/babe/types/seal.hpp"
#include "network/block_announce_transmitter.hpp"
#include "network/helpers/peer_id_formatter.hpp"
#include "network/synchronizer.hpp"
#include "network/types/block_announce.hpp"
#include "primitives/inherent_data.hpp"
#include "runtime/runtime_api/core.hpp"
#include "runtime/runtime_api/offchain_worker_api.hpp"
#include "scale/scale.hpp"
#include "storage/trie/serialization/ordered_trie_hash.hpp"

namespace {
  constexpr const char *kBlockProposalTime =
      "kagome_proposer_block_constructed";
}

using namespace std::literals::chrono_literals;

namespace kagome::consensus::babe {
  using SyncMethod = application::AppConfiguration::SyncMethod;

  BabeImpl::BabeImpl(
      const application::AppConfiguration &app_config,
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<BabeLottery> lottery,
      std::shared_ptr<consensus::babe::BabeConfigRepository> babe_config_repo,
      std::shared_ptr<authorship::Proposer> proposer,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<network::BlockAnnounceTransmitter>
          block_announce_transmitter,
      std::shared_ptr<crypto::Sr25519Provider> sr25519_provider,
      const std::shared_ptr<crypto::Sr25519Keypair> &keypair,
      std::shared_ptr<clock::SystemClock> clock,
      std::shared_ptr<crypto::Hasher> hasher,
      std::unique_ptr<clock::Timer> timer,
      std::shared_ptr<consensus::grandpa::GrandpaDigestObserver>
          grandpa_digest_observer,
      std::shared_ptr<network::Synchronizer> synchronizer,
      std::shared_ptr<BabeUtil> babe_util,
      primitives::events::ChainSubscriptionEnginePtr chain_events_engine,
      std::shared_ptr<runtime::OffchainWorkerApi> offchain_worker_api,
      std::shared_ptr<runtime::Core> core,
      std::shared_ptr<babe::ConsistencyKeeper> consistency_keeper)
      : app_config_(app_config),
        lottery_{std::move(lottery)},
        babe_config_repo_{std::move(babe_config_repo)},
        proposer_{std::move(proposer)},
        block_tree_{std::move(block_tree)},
        block_announce_transmitter_{std::move(block_announce_transmitter)},
        keypair_{keypair},
        clock_{std::move(clock)},
        hasher_{std::move(hasher)},
        sr25519_provider_{std::move(sr25519_provider)},
        timer_{std::move(timer)},
        grandpa_digest_observer_(std::move(grandpa_digest_observer)),
        synchronizer_(std::move(synchronizer)),
        babe_util_(std::move(babe_util)),
        chain_events_engine_(std::move(chain_events_engine)),
        chain_sub_([&] {
          BOOST_ASSERT(chain_events_engine_ != nullptr);
          return std::make_shared<primitives::events::ChainEventSubscriber>(
              chain_events_engine_);
        }()),
        offchain_worker_api_(std::move(offchain_worker_api)),
        runtime_core_(std::move(core)),
        consistency_keeper_(std::move(consistency_keeper)),
        log_{log::createLogger("Babe", "babe")},
        telemetry_{telemetry::createTelemetryService()} {
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
    BOOST_ASSERT(grandpa_digest_observer_);
    BOOST_ASSERT(synchronizer_);
    BOOST_ASSERT(babe_util_);
    BOOST_ASSERT(offchain_worker_api_);
    BOOST_ASSERT(runtime_core_);
    BOOST_ASSERT(consistency_keeper_);

    BOOST_ASSERT(app_state_manager);

    // Register metrics
    metrics_registry_->registerHistogramFamily(
        kBlockProposalTime, "Time taken to construct new block");
    metric_block_proposal_time_ = metrics_registry_->registerHistogramMetric(
        kBlockProposalTime,
        {0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1, 2.5, 5, 10});

    app_state_manager->takeControl(*this);
  }

  bool BabeImpl::prepare() {
    auto res = getInitialEpochDescriptor();
    if (res.has_error()) {
      SL_CRITICAL(log_, "Can't get initial epoch descriptor: {}", res.error());
      return false;
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
    best_block_ = block_tree_->deepestLeaf();

    SL_DEBUG(log_, "Babe is starting with syncing from block {}", best_block_);

    SL_DEBUG(log_,
             "Starting in epoch {} and slot {}",
             current_epoch_.epoch_number,
             current_epoch_.start_slot);

    if (keypair_) {
      auto babe_config =
          babe_config_repo_->config(best_block_, current_epoch_.epoch_number);
      if (babe_config == nullptr) {
        SL_CRITICAL(
            log_,
            "Can't obtain digest of epoch {} from block tree for block {}",
            current_epoch_.epoch_number,
            best_block_);
        return false;
      }

      const auto &authorities = babe_config->authorities;
      if (authorities.size() == 1
          and authorities[0].id.id == keypair_->public_key) {
        SL_INFO(log_, "Starting single validating node.");
        onSynchronized();
        return true;
      }
    }

    switch (app_config_.syncMethod()) {
      case SyncMethod::Full:
        current_state_ = State::WAIT_REMOTE_STATUS;
        break;

      case SyncMethod::Fast:
        if (synchronizer_->hasIncompleteRequestOfStateSync()) {
          // Has incomplete downloading state; continue loading of state
          current_state_ = State::STATE_LOADING;
        } else {
          // No incomplete downloading state; load headers first
          current_state_ = State::HEADERS_LOADING;
        }
        break;
    }

    return true;
  }

  void BabeImpl::stop() {}

  /**
   * @brief Get index of authority
   * @param authorities list of authorities
   * @param authority_key authority
   * @return index of authority in list of authorities
   */
  std::optional<uint64_t> getAuthorityIndex(
      const primitives::AuthorityList &authorities,
      const primitives::BabeSessionKey &authority_key) {
    uint64_t n = 0;
    for (auto &authority : authorities) {
      if (authority.id.id == authority_key) {
        return n;
      }
      ++n;
    }
    return std::nullopt;
  }

  outcome::result<EpochDescriptor> BabeImpl::getInitialEpochDescriptor() {
    auto best_block = block_tree_->deepestLeaf();

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
      auto res = block_tree_->getBlockHeader(primitives::BlockNumber(1));
      if (res.has_error()) {
        SL_TRACE(log_,
                 "First block slot is {}: no first block (at adjusting)",
                 babe_util_->getCurrentSlot());
        return std::tuple(babe_util_->getCurrentSlot(), false);
      }

      const auto &first_block_header = res.value();
      auto babe_digest_res = consensus::getBabeDigests(first_block_header);
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

    BOOST_ASSERT(keypair_ != nullptr);

    adjustEpochDescriptor();

    SL_DEBUG(
        log_,
        "Starting an epoch {}. Session key: {:l}. Secondary slots allowed={}",
        epoch.epoch_number,
        keypair_->public_key,
        babe_config_repo_->config(best_block_, epoch.epoch_number)
            ->isSecondarySlotsAllowed());
    current_epoch_ = epoch;
    current_slot_ = current_epoch_.start_slot;

    runSlot();
  }

  Babe::State BabeImpl::getCurrentState() const {
    return current_state_;
  }

  void BabeImpl::onRemoteStatus(const libp2p::peer::PeerId &peer_id,
                                const network::Status &status) {
    // If state is loading, just to ping of loading
    if (current_state_ == Babe::State::STATE_LOADING) {
      startStateSyncing(peer_id);
      return;
    }

    const auto &last_finalized_block = block_tree_->getLastFinalized();

    auto current_best_block_res =
        block_tree_->getBestContaining(last_finalized_block.hash, std::nullopt);
    BOOST_ASSERT(current_best_block_res.has_value());
    const auto &current_best_block = current_best_block_res.value();

    if (current_best_block == status.best_block) {
      if (current_state_ == Babe::State::HEADERS_LOADING) {
        current_state_ = Babe::State::HEADERS_LOADED;
        startStateSyncing(peer_id);
      } else if (current_state_ == Babe::State::CATCHING_UP
                 or current_state_ == Babe::State::WAIT_REMOTE_STATUS) {
        onSynchronized();
      }
      return;
    }

    // Remote peer is lagged
    if (status.best_block.number <= last_finalized_block.number) {
      return;
    }

    startCatchUp(peer_id, status.best_block);
  }

  void BabeImpl::onBlockAnnounce(const libp2p::peer::PeerId &peer_id,
                                 const network::BlockAnnounce &announce) {
    // If state is loading, just to ping of loading
    if (current_state_ == Babe::State::STATE_LOADING) {
      startStateSyncing(peer_id);
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
              self->startStateSyncing(peer_id);
              return;
            }

            // Just synced
            if (self->current_state_ == Babe::State::CATCHING_UP) {
              SL_INFO(self->log_, "Catching up is finished on block {}", block);
              self->current_state_ = Babe::State::SYNCHRONIZED;
              self->was_synchronized_ = true;
              self->telemetry_->notifyWasSynchronized();
            }

            // Synced
            if (self->current_state_ == Babe::State::SYNCHRONIZED) {
              self->onSynchronized();
              // Propagate announce
              self->block_announce_transmitter_->blockAnnounce(
                  std::move(announce));
              return;
            }
          }
        });
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
      } else if (current_state_ == State::WAIT_BLOCK_ANNOUNCE
                 or current_state_ == State::WAIT_REMOTE_STATUS
                 or current_state_ == State::SYNCHRONIZED) {
        current_state_ = State::CATCHING_UP;
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
        if (hash == block_at_state.hash) continue;

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

            SL_INFO(self->log_,
                    "State on block {} is synced successfully",
                    block_at_state);
            self->current_state_ = Babe::State::CATCHING_UP;
          }
        });
  }

  void BabeImpl::onSynchronized() {
    // won't start block production without keypair
    if (not keypair_) {
      current_state_ = State::WAIT_BLOCK_ANNOUNCE;
      return;
    }

    current_state_ = State::SYNCHRONIZED;
    was_synchronized_ = true;
    telemetry_->notifyWasSynchronized();

    if (not active_) {
      best_block_ = block_tree_->deepestLeaf();

      SL_DEBUG(log_, "Babe is synchronized on block {}", best_block_);

      runEpoch(current_epoch_);
    }
  }

  bool BabeImpl::wasSynchronized() const {
    return was_synchronized_;
  }

  void BabeImpl::runSlot() {
    BOOST_ASSERT(keypair_ != nullptr);

    bool rewind_slots;  // NOLINT
    auto slot = current_slot_;

    do {
      // check that we are really in the middle of the slot, as expected; we
      // can cooperate with a relatively little (kMaxLatency) latency, as our
      // node will be able to retrieve
      auto now = clock_->now();

      auto finish_time = babe_util_->slotFinishTime(current_slot_);

      rewind_slots =
          now > finish_time
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

    // Slot processing begins in 1/3 slot time before end
    auto finish_time = babe_util_->slotFinishTime(current_slot_)
                       - babe_config_repo_->slotDuration() / 3;

    SL_VERBOSE(log_,
               "Starting a slot {} in epoch {} (remains {:.2f} sec.)",
               current_slot_,
               current_epoch_.epoch_number,
               std::chrono::duration_cast<std::chrono::milliseconds>(
                   babe_util_->remainToFinishOfSlot(current_slot_))
                       .count()
                   / 1000.);

    // everything is OK: wait for the end of the slot
    timer_->expiresAt(finish_time);
    timer_->asyncWait([this](auto &&ec) {
      if (ec) {
        log_->error("error happened while waiting on the timer: {}", ec);
        return;
      }
      processSlot();
    });
  }

  void BabeImpl::processSlot() {
    BOOST_ASSERT(keypair_ != nullptr);

    best_block_ = block_tree_->deepestLeaf();

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

    auto babe_config =
        babe_config_repo_->config(best_block_, current_epoch_.epoch_number);
    if (babe_config) {
      auto authority_index_res =
          getAuthorityIndex(babe_config->authorities, keypair_->public_key);
      if (not authority_index_res) {
        SL_ERROR(log_,
                 "Authority not known, skipping slot processing. "
                 "Probably authority list has changed.");
      } else {
        const auto &authority_index = authority_index_res.value();

        if (lottery_->getEpoch() != current_epoch_) {
          changeLotteryEpoch(current_epoch_, babe_config);
        }

        auto slot_leadership = lottery_->getSlotLeadership(current_slot_);

        if (slot_leadership.has_value()) {
          const auto &vrf_result = slot_leadership.value();
          SL_DEBUG(log_,
                   "Babe author {} is leader (vrfOutput: {}, proof: {})",
                   keypair_->public_key,
                   common::Buffer(vrf_result.output),
                   common::Buffer(vrf_result.proof));

          processSlotLeadership(
              SlotType::Primary, std::cref(vrf_result), authority_index);
        } else if (babe_config->allowed_slots
                       == primitives::AllowedSlots::PrimaryAndSecondaryPlain
                   or babe_config->allowed_slots
                          == primitives::AllowedSlots::PrimaryAndSecondaryVRF) {
          auto expected_author =
              lottery_->secondarySlotAuthor(current_slot_,
                                            babe_config->authorities.size(),
                                            babe_config->randomness);

          if (expected_author.has_value()
              and authority_index == expected_author.value()) {
            if (babe_config->allowed_slots
                == primitives::AllowedSlots::PrimaryAndSecondaryVRF) {
              auto vrf = lottery_->slotVrfSignature(current_slot_);
              processSlotLeadership(
                  SlotType::SecondaryVRF, std::cref(vrf), authority_index);
            } else {  // plain secondary slots mode
              processSlotLeadership(
                  SlotType::SecondaryPlain, std::nullopt, authority_index);
            }
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
      std::optional<std::reference_wrapper<const crypto::VRFOutput>> output,
      primitives::AuthorityIndex authority_index) {
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
                   clock_->now().time_since_epoch())
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

    ParachainInherentData paras_inherent_data;

    // TODO: research of filling of bitfield, backed candidates, disputes
    //  issue https://github.com/soramitsu/kagome/issues/1209

    {
      auto best_block_header_res =
          block_tree_->getBlockHeader(best_block_.hash);
      BOOST_ASSERT_MSG(best_block_header_res.has_value(),
                       "The best block is always known");
      paras_inherent_data.parent_header =
          std::move(best_block_header_res.value());
    }

    if (auto res = inherent_data.putData(kParachainId, paras_inherent_data);
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

    // create new block
    auto pre_seal_block_res =
        proposer_->propose(best_block_, inherent_data, {babe_pre_digest});
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
    telemetry_->notifyBlockImported(block_info, telemetry::BlockOrigin::kOwn);

    // observe possible changes of authorities
    // (must be done strictly after block will be added)
    for (auto &digest_item : block.header.digest) {
      auto res = visit_in_place(
          digest_item,
          [&](const primitives::Consensus &consensus_message)
              -> outcome::result<void> {
            auto res = grandpa_digest_observer_->onDigest(block_info,
                                                          consensus_message);
            if (res.has_error()) {
              SL_WARN(log_,
                      "Can't process consensus message digest: {}",
                      res.error().message());
            }
            return res;
          },
          [](const auto &) { return outcome::success(); });
      if (res.has_error()) {
        return;
      }
    }

    // finally, broadcast the sealed block
    block_announce_transmitter_->blockAnnounce(
        network::BlockAnnounce{block.header});
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
      std::shared_ptr<const primitives::BabeConfiguration> babe_config) const {
    BOOST_ASSERT(keypair_ != nullptr);

    auto authority_index_res =
        getAuthorityIndex(babe_config->authorities, keypair_->public_key);
    if (not authority_index_res) {
      SL_CRITICAL(log_,
                  "Block production failed: This node is not in the list of "
                  "authorities. (public key: {})",
                  keypair_->public_key);
      return;
    }

    auto threshold = calculateThreshold(babe_config->leadership_rate,
                                        babe_config->authorities,
                                        authority_index_res.value());

    lottery_->changeEpoch(epoch, babe_config->randomness, threshold, *keypair_);
  }

  void BabeImpl::startNextEpoch() {
    SL_DEBUG(log_,
             "Epoch {} has finished. Start epoch {}",
             current_epoch_.epoch_number,
             current_epoch_.epoch_number + 1);

    ++current_epoch_.epoch_number;
    current_epoch_.start_slot = current_slot_;

    babe_util_->syncEpoch([&]() {
      auto res = block_tree_->getBlockHeader(primitives::BlockNumber(1));
      if (res.has_error()) {
        SL_WARN(log_,
                "First block slot is {}: no first block (at start next epoch)",
                babe_util_->getCurrentSlot());
        return std::tuple(babe_util_->getCurrentSlot(), false);
      }

      const auto &first_block_header = res.value();
      auto babe_digest_res = consensus::getBabeDigests(first_block_header);
      BOOST_ASSERT_MSG(babe_digest_res.has_value(),
                       "Any non genesis block must contain babe digest");
      auto first_slot_number = babe_digest_res.value().second.slot_number;

      auto is_first_block_finalized =
          block_tree_->getLastFinalized().number > 0;

      SL_WARN(log_,
              "First block slot is {}: by {}finalized first block (at start "
              "next epoch)",
              first_slot_number,
              is_first_block_finalized ? "" : "non-");
      return std::tuple(first_slot_number, is_first_block_finalized);
    });
  }

}  // namespace kagome::consensus::babe
