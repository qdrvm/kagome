/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/babe_impl.hpp"

#include <boost/assert.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include "blockchain/block_storage_error.hpp"
#include "blockchain/block_tree_error.hpp"
#include "common/buffer.hpp"
#include "consensus/babe/babe_error.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "consensus/babe/impl/threshold_util.hpp"
#include "consensus/babe/types/babe_block_header.hpp"
#include "consensus/babe/types/seal.hpp"
#include "network/block_announce_transmitter.hpp"
#include "network/synchronizer.hpp"
#include "network/types/block_announce.hpp"
#include "primitives/inherent_data.hpp"
#include "runtime/runtime_api/offchain_worker_api.hpp"
#include "scale/scale.hpp"
#include "storage/trie/serialization/ordered_trie_hash.hpp"

namespace {
  constexpr const char *kBlockProposalTime =
      "kagome_proposer_block_constructed";
}

using namespace std::literals::chrono_literals;

namespace kagome::consensus::babe {
  BabeImpl::BabeImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<BabeLottery> lottery,
      std::shared_ptr<primitives::BabeConfiguration> configuration,
      std::shared_ptr<authorship::Proposer> proposer,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<network::BlockAnnounceTransmitter>
          block_announce_transmitter,
      std::shared_ptr<crypto::Sr25519Provider> sr25519_provider,
      const std::shared_ptr<crypto::Sr25519Keypair> &keypair,
      std::shared_ptr<clock::SystemClock> clock,
      std::shared_ptr<crypto::Hasher> hasher,
      std::unique_ptr<clock::Timer> timer,
      std::shared_ptr<authority::AuthorityUpdateObserver>
          authority_update_observer,
      std::shared_ptr<network::Synchronizer> synchronizer,
      std::shared_ptr<BabeUtil> babe_util,
      std::shared_ptr<runtime::OffchainWorkerApi> offchain_worker_api)
      : was_synchronized_{false},
        lottery_{std::move(lottery)},
        babe_configuration_{std::move(configuration)},
        proposer_{std::move(proposer)},
        block_tree_{std::move(block_tree)},
        block_announce_transmitter_{std::move(block_announce_transmitter)},
        keypair_{keypair},
        clock_{std::move(clock)},
        hasher_{std::move(hasher)},
        sr25519_provider_{std::move(sr25519_provider)},
        timer_{std::move(timer)},
        authority_update_observer_(std::move(authority_update_observer)),
        synchronizer_(std::move(synchronizer)),
        babe_util_(std::move(babe_util)),
        offchain_worker_api_(std::move(offchain_worker_api)),
        log_{log::createLogger("Babe", "babe")} {
    BOOST_ASSERT(lottery_);
    BOOST_ASSERT(proposer_);
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(block_announce_transmitter_);
    BOOST_ASSERT(sr25519_provider_);
    BOOST_ASSERT(clock_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(timer_);
    BOOST_ASSERT(log_);
    BOOST_ASSERT(authority_update_observer_);
    BOOST_ASSERT(synchronizer_);
    BOOST_ASSERT(babe_util_);
    BOOST_ASSERT(offchain_worker_api_);

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
      SL_CRITICAL(log_,
                  "Can't get initial epoch descriptor: {}",
                  res.error().message());
      return false;
    }

    current_epoch_ = res.value();
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
      if (auto epoch_res = block_tree_->getEpochDigest(
              current_epoch_.epoch_number, best_block_.hash);
          epoch_res.has_value()) {
        const auto &authorities = epoch_res.value().authorities;
        if (authorities.size() == 1
            and authorities[0].id.id == keypair_->public_key) {
          SL_INFO(log_, "Starting single validating node.");
          onSynchronized();
          return true;
        }
      } else {
        SL_CRITICAL(
            log_,
            "Can't obtain digest of epoch {} from block tree for block {}: {}",
            current_epoch_.epoch_number,
            best_block_,
            epoch_res.error().message());
        return false;
      }
    }
    current_state_ = State::WAIT_REMOTE_STATUS;
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
                                          / babe_configuration_->slot_duration)
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

  void BabeImpl::runEpoch(EpochDescriptor epoch) {
    bool already_active = false;
    if (not active_.compare_exchange_strong(already_active, true)) {
      return;
    }

    BOOST_ASSERT(keypair_ != nullptr);

    SL_DEBUG(
        log_,
        "Starting an epoch {}. Session key: {}. Secondary slots allowed={}",
        epoch.epoch_number,
        keypair_->public_key.toHex(),
        isSecondarySlotsAllowed());
    current_epoch_ = epoch;
    current_slot_ = current_epoch_.start_slot;

    babe_util_->syncEpoch(current_epoch_);

    runSlot();
  }

  Babe::State BabeImpl::getCurrentState() const {
    return current_state_;
  }

  void BabeImpl::onRemoteStatus(const libp2p::peer::PeerId &peer_id,
                                const network::Status &status) {
    const auto &last_finalized_block = block_tree_->getLastFinalized();

    auto current_best_block_res =
        block_tree_->getBestContaining(last_finalized_block.hash, std::nullopt);
    BOOST_ASSERT(current_best_block_res.has_value());
    const auto &current_best_block = current_best_block_res.value();

    if (current_best_block == status.best_block) {
      onSynchronized();
      return;
    }

    // Remote peer is lagged
    if (current_best_block.number > status.best_block.number) {
      return;
    }

    startCatchUp(peer_id, status.best_block);
  }

  void BabeImpl::onBlockAnnounce(const libp2p::peer::PeerId &peer_id,
                                 const network::BlockAnnounce &announce) {
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
    if (announce.header.number > current_best_block.number + 1) {
      auto block_hash =
          hasher_->blake2b_256(scale::encode(announce.header).value());
      const primitives::BlockInfo announced_block(announce.header.number,
                                                  block_hash);
      startCatchUp(peer_id, announced_block);
      return;
    }

    // Received announce that has the same block number as ours best,
    // or greater by one. Using of simple way to load block
    synchronizer_->syncByBlockHeader(
        announce.header,
        peer_id,
        [wp = weak_from_this(), announce = announce](
            outcome::result<primitives::BlockInfo> block_res) mutable {
          if (auto self = wp.lock()) {
            if (block_res.has_error()) {
              return;
            }

            if (self->current_state_ == Babe::State::CATCHING_UP) {
              const auto &block = block_res.value();
              SL_INFO(self->log_, "Catching up is finished on block {}", block);
              self->current_state_ = Babe::State::SYNCHRONIZED;
              self->was_synchronized_ = true;
            }
            self->onSynchronized();

            // Propagate announce
            self->block_announce_transmitter_->blockAnnounce(
                std::move(announce));
          }
        });
  }

  void BabeImpl::startCatchUp(const libp2p::peer::PeerId &peer_id,
                              const primitives::BlockInfo &target_block) {
    // synchronize missing blocks with their bodies

    auto is_ran = synchronizer_->syncByBlockInfo(
        target_block,
        peer_id,
        [wp = weak_from_this(),
         bn = target_block.number](outcome::result<primitives::BlockInfo> res) {
          if (auto self = wp.lock()) {
            if (res.has_error()) {
              SL_INFO(self->log_,
                      "Catching up to block #{} is failed: {}",
                      bn,
                      res.error().message());
              return;
            }

            SL_INFO(self->log_,
                    "Catching up to block #{} is going (on block #{} now)",
                    bn,
                    res.value().number);
          }
        },
        false);

    if (is_ran) {
      SL_INFO(log_, "Catching up to block #{} is ran", target_block.number);
      current_state_ = State::CATCHING_UP;
    }
  }

  void BabeImpl::onSynchronized() {
    // won't start block production without keypair
    if (not keypair_) {
      current_state_ = State::WAIT_BLOCK_ANNOUNCE;
      return;
    }

    current_state_ = State::SYNCHRONIZED;
    was_synchronized_ = true;

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
          and (now - finish_time) > babe_configuration_->slot_duration;

      if (rewind_slots) {
        // we are too far behind; after skipping some slots (but not epochs)
        // control will be returned to this method

        ++current_slot_;

        if (current_epoch_.epoch_number
            != babe_util_->slotToEpoch(current_slot_)) {
          startNextEpoch();
        }
      } else if (slot < current_slot_) {
        SL_VERBOSE(log_, "Slots {}..{} was skipped", slot, current_slot_ - 1);
      }
    } while (rewind_slots);

    // Slot processing begins in 1/3 slot time before end
    auto finish_time = babe_util_->slotFinishTime(current_slot_)
                       - babe_util_->slotDuration() / 3;

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
        log_->error("error happened while waiting on the timer: {}",
                    ec.message());
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
        // Only genesis may not have a babe digest
        break;
      }
      BOOST_ASSERT(babe_digests_res.has_value());
    }

    auto epoch_res = block_tree_->getEpochDigest(current_epoch_.epoch_number,
                                                 best_block_.hash);
    if (epoch_res.has_error()) {
      SL_ERROR(log_,
               "Fail to get epoch: {}; Skipping slot processing",
               epoch_res.error().message());
      return;
    }
    const auto &epoch = epoch_res.value();

    auto authority_index_res =
        getAuthorityIndex(epoch.authorities, keypair_->public_key);
    if (not authority_index_res) {
      SL_ERROR(log_,
               "Authority not known, skipping slot processing. "
               "Probably authority list has changed.");
      return;
    }
    const auto &authority_index = authority_index_res.value();

    if (lottery_->getEpoch() != current_epoch_) {
      changeLotteryEpoch(current_epoch_, epoch.authorities, epoch.randomness);
    }

    auto slot_leadership = lottery_->getSlotLeadership(current_slot_);

    if (slot_leadership.has_value()) {
      const auto &vrf_result = slot_leadership.value();
      SL_DEBUG(log_,
               "Peer {} is leader (vrfOutput: {}, proof: {})",
               keypair_->public_key.toHex(),
               common::Buffer(vrf_result.output).toHex(),
               common::Buffer(vrf_result.proof).toHex());

      processSlotLeadership(
          SlotType::Primary, std::cref(vrf_result), authority_index);
    } else if (isSecondarySlotsAllowed()) {
      auto expected_author = lottery_->secondarySlotAuthor(
          current_slot_, epoch.authorities.size(), epoch.randomness);

      if (expected_author.has_value()
          and authority_index == expected_author.value()) {
        if (primitives::AllowedSlots::PrimaryAndSecondaryVRFSlots
            == babe_configuration_->allowed_slots) {
          auto vrf = lottery_->slotVrfSignature(current_slot_);
          processSlotLeadership(
              SlotType::SecondaryVRF, std::cref(vrf), authority_index);
        } else {  // plain secondary slots mode
          processSlotLeadership(
              SlotType::SecondaryPlain, std::nullopt, authority_index);
        }
      }
    }

    SL_DEBUG(log_,
             "Slot {} in epoch {} has finished",
             current_slot_,
             current_epoch_.epoch_number);

    ++current_slot_;

    if (current_epoch_.epoch_number != babe_util_->slotToEpoch(current_slot_)) {
      startNextEpoch();
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
        log_->error("error happened while waiting on the timer: {}",
                    ec.message());
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
        .slot_number = current_slot_,
        .authority_index = authority_index,
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
               encoded_header_res.error().message());
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
      SL_ERROR(
          log_, "Error signing a block seal: {}", signature.error().message());
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

    primitives::InherentData inherent_data;
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                   clock_->now().time_since_epoch())
                   .count();
    // identifiers are guaranteed to be correct, so use .value() directly
    auto put_res = inherent_data.putData<uint64_t>(kTimestampId, now);
    if (!put_res) {
      SL_ERROR(
          log_, "cannot put an inherent data: {}", put_res.error().message());
      return;
    }
    put_res = inherent_data.putData(kBabeSlotId, current_slot_);
    if (!put_res) {
      SL_ERROR(
          log_, "cannot put an inherent data: {}", put_res.error().message());
      return;
    }

    SL_INFO(log_, "Babe builds block on top of block {}", best_block_);

    auto proposal_start = std::chrono::high_resolution_clock::now();
    // calculate babe_pre_digest
    auto babe_pre_digest_res =
        babePreDigest(slot_type, output, authority_index);
    if (not babe_pre_digest_res) {
      SL_ERROR(log_,
               "cannot propose a block: {}",
               babe_pre_digest_res.error().message());
      return;
    }
    const auto &babe_pre_digest = babe_pre_digest_res.value();

    // create new block
    auto pre_seal_block_res =
        proposer_->propose(best_block_, inherent_data, {babe_pre_digest});
    if (!pre_seal_block_res) {
      SL_ERROR(log_,
               "Cannot propose a block: {}",
               pre_seal_block_res.error().message());
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
      SL_ERROR(
          log_, "Failed to seal the block: {}", seal_res.error().message());
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

    // observe possible changes of authorities
    for (auto &digest_item : block.header.digest) {
      auto res = visit_in_place(
          digest_item,
          [&](const primitives::Consensus &consensus_message)
              -> outcome::result<void> {
            auto res = authority_update_observer_->onConsensus(
                block_info, consensus_message);
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

    auto last_finalized_block = block_tree_->getLastFinalized();
    auto previous_best_block_res =
        block_tree_->getBestContaining(last_finalized_block.hash, std::nullopt);
    BOOST_ASSERT(previous_best_block_res.has_value());
    const auto &previous_best_block = previous_best_block_res.value();

    // add block to the block tree
    if (auto add_res = block_tree_->addBlock(block); not add_res) {
      SL_ERROR(log_,
               "Could not add block {}: {}",
               block_info,
               add_res.error().message());
      auto removal_res = block_tree_->removeBlock(block_hash);
      if (removal_res.has_error()
          and removal_res
                  != outcome::failure(
                      blockchain::BlockTreeError::BLOCK_IS_NOT_LEAF)) {
        SL_WARN(log_,
                "Rolling back of block {} is failed: {}",
                block_info,
                removal_res.error().message());
      }
      return;
    }

    if (auto next_epoch_digest_res = getNextEpochDigest(block.header);
        next_epoch_digest_res.has_value()) {
      auto &next_epoch_digest = next_epoch_digest_res.value();
      SL_VERBOSE(log_,
                 "Got next epoch digest in slot {} (block #{}). Randomness: {}",
                 current_slot_,
                 block.header.number,
                 next_epoch_digest.randomness.toHex());
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
                    ocw_res.error().message());
      }
    }
  }

  void BabeImpl::changeLotteryEpoch(
      const EpochDescriptor &epoch,
      const primitives::AuthorityList &authorities,
      const Randomness &randomness) const {
    BOOST_ASSERT(keypair_ != nullptr);

    auto authority_index_res =
        getAuthorityIndex(authorities, keypair_->public_key);
    if (not authority_index_res) {
      SL_CRITICAL(log_,
                  "Block production failed: This node is not in the list of "
                  "authorities. (public key: {})",
                  keypair_->public_key.toHex());
      return;
    }

    auto threshold = calculateThreshold(babe_configuration_->leadership_rate,
                                        authorities,
                                        authority_index_res.value());

    lottery_->changeEpoch(epoch, randomness, threshold, *keypair_);
  }

  void BabeImpl::startNextEpoch() {
    SL_DEBUG(log_,
             "Epoch {} has finished. Start epoch {}",
             current_epoch_.epoch_number,
             current_epoch_.epoch_number + 1);

    ++current_epoch_.epoch_number;
    current_epoch_.start_slot = current_slot_;

    babe_util_->syncEpoch(current_epoch_);
  }

  bool BabeImpl::isSecondarySlotsAllowed() const {
    return primitives::AllowedSlots::PrimaryAndSecondaryPlainSlots
               == babe_configuration_->allowed_slots
           or primitives::AllowedSlots::PrimaryAndSecondaryVRFSlots
                  == babe_configuration_->allowed_slots;
  }

}  // namespace kagome::consensus::babe
