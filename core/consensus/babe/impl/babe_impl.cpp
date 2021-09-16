/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/babe_impl.hpp"

#include <boost/assert.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include "blockchain/block_tree_error.hpp"
#include "clock/impl/ticker_impl.hpp"
#include "common/buffer.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "consensus/babe/impl/threshold_util.hpp"
#include "consensus/babe/types/babe_block_header.hpp"
#include "consensus/babe/types/seal.hpp"
#include "network/block_announce_transmitter.hpp"
#include "network/types/block_announce.hpp"
#include "primitives/inherent_data.hpp"
#include "scale/scale.hpp"
#include "storage/trie/serialization/ordered_trie_hash.hpp"

using namespace std::literals::chrono_literals;

namespace kagome::consensus::babe {
  BabeImpl::BabeImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<BabeLottery> lottery,
      std::shared_ptr<BlockExecutor> block_executor,
      std::shared_ptr<storage::trie::TrieStorage> trie_storage,
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
      std::shared_ptr<BabeUtil> babe_util)
      : lottery_{std::move(lottery)},
        block_executor_{std::move(block_executor)},
        trie_storage_{std::move(trie_storage)},
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
        babe_util_(std::move(babe_util)),
        log_{log::createLogger("Babe", "babe")} {
    BOOST_ASSERT(lottery_);
    BOOST_ASSERT(trie_storage_);
    BOOST_ASSERT(proposer_);
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(block_announce_transmitter_);
    BOOST_ASSERT(sr25519_provider_);
    BOOST_ASSERT(clock_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(timer_);
    BOOST_ASSERT(log_);
    BOOST_ASSERT(authority_update_observer_);
    BOOST_ASSERT(babe_util_);

    BOOST_ASSERT(app_state_manager);
    app_state_manager->takeControl(*this);
  }

  bool BabeImpl::prepare() {
    return true;
  }

  bool BabeImpl::start() {
    best_block_ = block_tree_->deepestLeaf();

    SL_DEBUG(log_,
             "Babe is starting with syncing from block #{}, hash={}",
             best_block_.number,
             best_block_.hash);

    EpochDescriptor last_epoch_descriptor;
    const auto now = clock_->now();
    if (auto res = babe_util_->getLastEpoch(); res.has_value()) {
      last_epoch_descriptor = res.value();

      SL_DEBUG(log_,
               "Pre-saved epoch {} (started in slot {})",
               last_epoch_descriptor.epoch_number,
               last_epoch_descriptor.start_slot);

    } else {
      last_epoch_descriptor.epoch_number = 0;
      last_epoch_descriptor.start_slot =
          static_cast<BabeSlotNumber>(now.time_since_epoch()
                                      / babe_configuration_->slot_duration)
          + 1;

      SL_DEBUG(log_,
               "Temporary epoch {} (started in slot {})",
               last_epoch_descriptor.epoch_number,
               last_epoch_descriptor.start_slot);
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
  boost::optional<uint64_t> getAuthorityIndex(
      const primitives::AuthorityList &authorities,
      const primitives::BabeSessionKey &authority_key) {
    uint64_t n = 0;
    for (auto &authority : authorities) {
      if (authority.id.id == authority_key) {
        return n;
      }
      ++n;
    }
    return boost::none;
  }

  void BabeImpl::runEpoch(EpochDescriptor epoch) {
    bool already_active = false;
    if (not active_.compare_exchange_strong(already_active, true)) {
      return;
    }

    BOOST_ASSERT(keypair_ != nullptr);

    SL_DEBUG(log_,
             "Starting an epoch {}. Session key: {}",
             epoch.epoch_number,
             keypair_->public_key.toHex());
    current_epoch_ = std::move(epoch);
    current_slot_ = current_epoch_.start_slot;

    [[maybe_unused]] auto res = babe_util_->setLastEpoch(current_epoch_);

    runSlot();
  }

  Babe::State BabeImpl::getCurrentState() const {
    return current_state_;
  }

  void BabeImpl::onRemoteStatus(const libp2p::peer::PeerId &peer_id,
                                const network::Status &status) {
    if (current_state_ == State::WAIT_REMOTE_STATUS
        or current_state_ == State::WAIT_BLOCK_ANNOUNCE) {
      const auto &last_finalized_block = block_tree_->getLastFinalized();

      auto current_best_block_res = block_tree_->getBestContaining(
          last_finalized_block.hash, boost::none);
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

      startCatchUp(
          peer_id, last_finalized_block, current_best_block, status.best_block);
    }
  }

  void BabeImpl::onBlockAnnounce(const libp2p::peer::PeerId &peer_id,
                                 const network::BlockAnnounce &announce) {
    if (current_state_ == State::WAIT_BLOCK_ANNOUNCE
        or current_state_ == State::SYNCHRONIZED) {
      const auto &last_finalized_block = block_tree_->getLastFinalized();

      auto current_best_block_res = block_tree_->getBestContaining(
          last_finalized_block.hash, boost::none);
      BOOST_ASSERT(current_best_block_res.has_value());
      const auto &current_best_block = current_best_block_res.value();

      auto block_hash =
          hasher_->blake2b_256(scale::encode(announce.header).value());
      const primitives::BlockInfo announced_block(announce.header.number,
                                                  block_hash);

      // Skip obsoleted announce
      if (announced_block.number < current_best_block.number) {
        return;
      }

      // Start catching up if gap recognized
      if (announced_block.number > current_best_block.number + 1) {
        startCatchUp(
            peer_id, last_finalized_block, current_best_block, announced_block);
      }

      // Already has block with same number as ours best block or next of that
      block_executor_->processNextBlock(
          peer_id, announce.header, [this](const auto &header) {
            onSynchronized();
          });
    }
  }

  void BabeImpl::startCatchUp(const libp2p::peer::PeerId &peer_id,
                              const primitives::BlockInfo &finalized_block,
                              const primitives::BlockInfo &best_block,
                              const primitives::BlockInfo &target_block) {
    // synchronize missing blocks with their bodies
    SL_INFO(log_, "Catching up to block #{} is ran", target_block.number);
    current_state_ = State::CATCHING_UP;

    block_executor_->requestBlocks(
        finalized_block.hash,
        target_block.hash,
        peer_id,
        [wp = weak_from_this(), bn = target_block.number] {
          if (auto self = wp.lock()) {
            SL_INFO(self->log_, "Catching up to block #{} is done", bn);
            self->current_state_ = State::WAIT_BLOCK_ANNOUNCE;
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

    if (not active_) {
      EpochDescriptor last_epoch_descriptor;
      if (auto res = babe_util_->getLastEpoch(); res.has_value()) {
        last_epoch_descriptor = res.value();
      } else {
        last_epoch_descriptor.epoch_number = 0;
        last_epoch_descriptor.start_slot = babe_util_->getCurrentSlot() + 1;
      }

      best_block_ = block_tree_->deepestLeaf();

      SL_DEBUG(log_,
               "Babe is synchronized on block #{}, hash={}",
               best_block_.number,
               best_block_.hash);

      runEpoch(last_epoch_descriptor);
      on_synchronized_();
    }
  }

  void BabeImpl::doOnSynchronized(std::function<void()> handler) {
    if (current_state_ == State::SYNCHRONIZED) {
      handler();
    } else {
      on_synchronized_ = std::move(handler);
    }
  }

  void BabeImpl::runSlot() {
    BOOST_ASSERT(keypair_ != nullptr);

    bool rewind_slots;  // NOLINT
    auto slot = current_slot_;
    do {
      // check that we are really in the middle of the slot, as expected; we can
      // cooperate with a relatively little (kMaxLatency) latency, as our node
      // will be able to retrieve
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
        log_->info("Slots {}..{} was skipped", slot, current_slot_ - 1);
      }
    } while (rewind_slots);

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
        // Shift to parent block and check again
        best_block_ =
            primitives::BlockInfo(header.number - 1, header.parent_hash);
        continue;

      } else if (best_block_.number == 0) {
        // Only genesis may not have a babe digest
        break;
      }
      BOOST_ASSERT(babe_digests_res.has_value());
    }

    // Slot processing begins in 1/3 slot time before end
    auto finish_time = babe_util_->slotFinishTime(current_slot_)
                       - babe_util_->slotDuration() / 3;

    log_->info("Starting a slot {} in epoch {} (remains {:.2f} sec.)",
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

    if (lottery_->getEpoch() != current_epoch_) {
      auto epoch_res = block_tree_->getEpochDescriptor(
          current_epoch_.epoch_number, best_block_.hash);
      BOOST_ASSERT(epoch_res.has_value());
      const auto &epoch = epoch_res.value();

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

      processSlotLeadership(vrf_result);
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

    log_->info("Slot {} in epoch {} will start after {:.2f} sec.",
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
      const crypto::VRFOutput &output,
      primitives::AuthorityIndex authority_index) const {
    BabeBlockHeader babe_header{
        BabeBlockHeader::kVRFHeader, current_slot_, output, authority_index};
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

  void BabeImpl::processSlotLeadership(const crypto::VRFOutput &output) {
    BOOST_ASSERT(keypair_ != nullptr);

    // build a block to be announced
    SL_INFO(log_,
            "Obtained slot leadership in slot {} epoch {}",
            current_slot_,
            current_epoch_.epoch_number);

    primitives::InherentData inherent_data;
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                   clock_->now().time_since_epoch())
                   .count();
    // identifiers are guaranteed to be correct, so use .value() directly
    auto put_res = inherent_data.putData<uint64_t>(kTimestampId, now);
    if (!put_res) {
      return SL_ERROR(
          log_, "cannot put an inherent data: {}", put_res.error().message());
    }
    put_res = inherent_data.putData(kBabeSlotId, current_slot_);
    if (!put_res) {
      return SL_ERROR(
          log_, "cannot put an inherent data: {}", put_res.error().message());
    }

    SL_INFO(log_,
            "Babe builds block on top of block with number {} and hash {}",
            best_block_.number,
            best_block_.hash);

    auto epoch = block_tree_->getEpochDescriptor(current_epoch_.epoch_number,
                                                 best_block_.hash);

    auto authority_index_res =
        getAuthorityIndex(epoch.value().authorities, keypair_->public_key);
    BOOST_ASSERT_MSG(authority_index_res.has_value(), "Authority is not known");

    // calculate babe_pre_digest
    auto babe_pre_digest_res =
        babePreDigest(output, authority_index_res.value());
    if (not babe_pre_digest_res) {
      return SL_ERROR(log_,
                      "cannot propose a block: {}",
                      babe_pre_digest_res.error().message());
    }
    const auto &babe_pre_digest = babe_pre_digest_res.value();

    // create new block
    auto pre_seal_block_res = proposer_->propose(
        best_block_.number, inherent_data, {babe_pre_digest});
    if (!pre_seal_block_res) {
      return SL_ERROR(log_,
                      "Cannot propose a block: {}",
                      pre_seal_block_res.error().message());
    }

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
      return SL_ERROR(
          log_, "Failed to seal the block: {}", seal_res.error().message());
    }

    // add seal digest item
    block.header.digest.emplace_back(seal_res.value());

    // check that we are still in the middle of the
    if (babe_util_->remainToFinishOfSlot(current_slot_ + kMaxBlockSlotsOvertime)
            .count()
        == 0) {
      SL_WARN(
          log_,
          "Block was not built in time. "
          "Allowed slots ({}) have passed. "
          "If you are executing in debug mode, consider to rebuild in release",
          kMaxBlockSlotsOvertime);
      return;
    }

    // observe possible changes of authorities
    for (auto &digest_item : block.header.digest) {
      visit_in_place(
          digest_item,
          [&](const primitives::Consensus &consensus_message) {
            [[maybe_unused]] auto res = authority_update_observer_->onConsensus(
                consensus_message.consensus_engine_id,
                best_block_,
                consensus_message);
          },
          [](const auto &) {});
    }

    // add block to the block tree
    if (auto add_res = block_tree_->addBlock(block); not add_res) {
      SL_ERROR(log_, "Could not add block: {}", add_res.error().message());
      return;
    }

    if (auto next_epoch_digest_res = getNextEpochDigest(block.header);
        next_epoch_digest_res) {
      auto &next_epoch_digest = next_epoch_digest_res.value();
      SL_INFO(log_,
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

    [[maybe_unused]] auto res = babe_util_->setLastEpoch(
        {current_epoch_.epoch_number, current_epoch_.start_slot});
  }
}  // namespace kagome::consensus::babe
