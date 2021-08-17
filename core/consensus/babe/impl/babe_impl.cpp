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

// a period of time to check key appearance when absent
constexpr auto kKeyWaitTimerDuration = 60s;

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
      std::shared_ptr<boost::asio::io_context> io_context,
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
    BOOST_ASSERT(log_);
    BOOST_ASSERT(authority_update_observer_);
    BOOST_ASSERT(babe_util_);

    BOOST_ASSERT(app_state_manager);
    app_state_manager->takeControl(*this);

    ticker_ = std::make_unique<clock::TickerImpl>(
        io_context, babe_configuration_->slot_duration);
    key_wait_ticker_ =
        std::make_unique<clock::TickerImpl>(io_context, kKeyWaitTimerDuration);
  }

  bool BabeImpl::prepare() {
    return true;
  }

  bool BabeImpl::start() {
    current_state_ = State::WAIT_BLOCK;
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
    BOOST_ASSERT(keypair_ != nullptr);

    SL_DEBUG(log_,
             "Starting an epoch {}. Session key: {}",
             epoch.epoch_number,
             keypair_->public_key.toHex());
    current_epoch_ = epoch;
    current_slot_ = current_epoch_.start_slot;

    [[maybe_unused]] auto res = babe_util_->setLastEpoch(current_epoch_);

    // main babe block production loop is here
    ticker_->asyncCallRepeatedly([wp = weak_from_this()](auto &&ec) {
      if (auto self = wp.lock()) {
        SL_INFO(self->log_,
                "Starting a slot {} in epoch {}",
                self->current_slot_,
                self->current_epoch_.epoch_number);
        if (ec) {
          SL_ERROR(self->log_,
                   "error happened while waiting on the ticker: {}",
                   ec.message());
          return;
        }
        self->processSlot();
      }
    });
    auto slot_start_delay = babe_util_->slotStartsIn(current_slot_);
    auto delay_shift = babe_util_->slotDuration() / 3 * 2;
    /*
     * The delay lets us start slot processing when 2/3 of the time of the slot
     * duration passed, thus more transactions could have been arrived to be
     * baked into the block.
     */
    auto shifted_slot_start_delay = slot_start_delay + delay_shift;
    auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(
                    shifted_slot_start_delay)
                    .count();
    SL_TRACE(log_, "Babe starts in {} msec", msec);
    ticker_->start(shifted_slot_start_delay);
  }

  Babe::State BabeImpl::getCurrentState() const {
    return current_state_;
  }

  void BabeImpl::onRemoteStatus(const libp2p::peer::PeerId &peer_id,
                                const network::Status &status) {
    if (current_state_ == State::WAIT_BLOCK) {
      if (status.best_block.number == 1) {
        current_state_ = State::SYNCHRONIZED;
        return;
      }

      const auto &last_finalized_block = block_tree_->getLastFinalized();

      auto current_best_block_res = block_tree_->getBestContaining(
          last_finalized_block.hash, boost::none);
      BOOST_ASSERT(current_best_block_res.has_value());
      const auto &current_best_block = current_best_block_res.value();

      if (current_best_block.number > status.best_block.number) {
        return;
      }

      // synchronize missing blocks with their bodies
      SL_INFO(
          log_, "Catching up to block number: {}", status.best_block.number);
      current_state_ = State::CATCHING_UP;

      block_executor_->requestBlocks(
          last_finalized_block.hash,
          status.best_block.hash,
          peer_id,
          [wp = weak_from_this()] {
            if (auto self = wp.lock()) {
              SL_INFO(self->log_, "Catching up is done");
              self->current_state_ = State::SYNCHRONIZED;
            }
          });
    }
  }

  void BabeImpl::onBlockAnnounce(const libp2p::peer::PeerId &peer_id,
                                 const network::BlockAnnounce &announce) {
    switch (current_state_) {
      case State::WAIT_BLOCK:
        // TODO(kamilsa): PRE-366 validate block. Now it is problematic as we
        //  need t know VRF threshold for validation. To calculate that we need
        //  to have weights of the authorities and to get it we need to have
        //  the latest state of a blockchain

        // synchronize missing blocks with their bodies
        SL_INFO(
            log_, "Catching up to block number: {}", announce.header.number);
        current_state_ = State::CATCHING_UP;
        block_executor_->requestBlocks(
            peer_id, announce.header, [wp = weak_from_this()] {
              if (auto self = wp.lock()) {
                SL_INFO(self->log_, "Catching up is done");
                self->current_state_ = State::SYNCHRONIZED;
              }
            });
        break;
      case State::CATCHING_UP:
      case State::SYNCHRONIZED:
        block_executor_->processNextBlock(
            peer_id, announce.header, [this](const auto &header) {
              synchronizeSlots(header);
            });
        break;
    }
  }

  void BabeImpl::onPeerSync() {
    // sync already started no need to call it again
    if (ticker_->isStarted()) {
      // stop key wait ticker when key appeared if started
      if (key_wait_ticker_->isStarted()) {
        key_wait_ticker_->stop();
      }
      return;
    }

    // won't start block production without keypair
    if (not keypair_) {
      // launch timer that would check key appearance
      if (not key_wait_ticker_->isStarted()) {
        key_wait_ticker_->asyncCallRepeatedly(
            [wp = weak_from_this()](auto &&ec) {
              if (auto self = wp.lock()) {
                if (ec) {
                  if (ec.value() == boost::system::errc::operation_canceled) {
                    SL_INFO(self->log_, "key_wait_ticker {}", ec.message());
                    return;
                  }
                  SL_ERROR(
                      self->log_,
                      "error happened while waiting on the key_wait_ticker: {}",
                      ec.message());
                  return;
                }
                SL_INFO(self->log_, "Check if babe key appeared...");
                self->onPeerSync();
              }
            });
        key_wait_ticker_->start(key_wait_ticker_->interval());
      }
      return;
    }

    current_state_ = State::SYNCHRONIZED;

    EpochDescriptor last_epoch_descriptor;
    if (auto res = babe_util_->getLastEpoch(); res.has_value()) {
      last_epoch_descriptor = res.value();
    } else {
      last_epoch_descriptor.epoch_number = 0;
      last_epoch_descriptor.start_slot = babe_util_->getCurrentSlot() + 1;
    }

    auto [number, hash] = block_tree_->deepestLeaf();
    auto &best_block_number = number;
    auto &best_block_hash = hash;

    SL_DEBUG(log_,
             "Babe is starting with syncing from block #{}, hash={}",
             best_block_number,
             best_block_hash);

    runEpoch(last_epoch_descriptor);
    on_synchronized_();
  }

  void BabeImpl::doOnSynchronized(std::function<void()> handler) {
    if (current_state_ == State::SYNCHRONIZED) {
      handler();
    } else {
      on_synchronized_ = std::move(handler);
    }
  }

  void BabeImpl::setTicker(std::unique_ptr<clock::Ticker> &&ticker) {
    ticker_ = std::move(ticker);
  }

  void BabeImpl::processSlot() {
    BOOST_ASSERT(keypair_ != nullptr);

    if (lottery_->getEpoch() != current_epoch_) {
      const auto &[_, best_block_hash] = block_tree_->deepestLeaf();

      auto epoch_res = block_tree_->getEpochDescriptor(
          current_epoch_.epoch_number, best_block_hash);
      BOOST_ASSERT(epoch_res.has_value());
      const auto &epoch = epoch_res.value();

      changeLotteryEpoch(current_epoch_, epoch.authorities, epoch.randomness);
    }

    auto slot_leadership = lottery_->getSlotLeadership(current_slot_);

    if (slot_leadership) {
      SL_DEBUG(log_,
               "Peer {} is leader (vrfOutput: {}, proof: {})",
               keypair_->public_key.toHex(),
               common::Buffer(slot_leadership->output).toHex(),
               common::Buffer(slot_leadership->proof).toHex());

      processSlotLeadership(*slot_leadership);
    }

    SL_DEBUG(log_,
             "Slot {} in epoch {} has finished",
             current_slot_,
             current_epoch_.epoch_number);

    current_slot_ = babe_util_->getCurrentSlot() + 1;

    if (current_epoch_.epoch_number != babe_util_->slotToEpoch(current_slot_)) {
      startNextEpoch();
    }
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

    auto best_block_info = block_tree_->deepestLeaf();
    auto &[best_block_number, best_block_hash] = best_block_info;
    SL_INFO(log_,
            "Babe builds block on top of block with number {} and hash {}",
            best_block_info.number,
            best_block_info.hash);

    auto epoch = block_tree_->getEpochDescriptor(current_epoch_.epoch_number,
                                                 best_block_hash);

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
    auto pre_seal_block_res =
        proposer_->propose(best_block_number, inherent_data, {babe_pre_digest});
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
    auto block_delay = babe_util_->getCurrentSlot() - current_slot_;
    if (block_delay > kMaxBlockSlotsOvertime) {
      SL_WARN(
          log_,
          "Block was not built in time. "
          "Allowed slots ({}) have passed. "
          "Block delayed for {} slots. "
          "If you are executing in debug mode, consider to rebuild in release",
          kMaxBlockSlotsOvertime,
          block_delay);
      return;
    }

    // observe possible changes of authorities
    for (auto &digest_item : block.header.digest) {
      visit_in_place(
          digest_item,
          [&](const primitives::Consensus &consensus_message) {
            [[maybe_unused]] auto res = authority_update_observer_->onConsensus(
                consensus_message.consensus_engine_id,
                best_block_info,
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
      throw boost::bad_optional_access();
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

  EpochDescriptor BabeImpl::prepareFirstEpoch(
      EpochDescriptor last_known_epoch,
      BabeSlotNumber first_production_slot) const {
    const auto epoch_duration = babe_configuration_->epoch_length;
    const auto start_slot =
        first_production_slot
        - ((first_production_slot - last_known_epoch.start_slot)
           % epoch_duration)
        + 1;
    const auto genesis_slot = babe_util_->getCurrentSlot();
    const auto epoch_number = (first_production_slot - genesis_slot)
                              / babe_configuration_->epoch_length;

    return EpochDescriptor{.epoch_number = epoch_number,
                           .start_slot = start_slot};
  }

  void BabeImpl::synchronizeSlots(const primitives::BlockHeader &new_header) {
    // skip logic without keypair, wait next block announce
    if (not keypair_) {
      return;
    }

    // runEpoch starts ticker
    if (not ticker_->isStarted()) {
      const auto &babe_digests_res = getBabeDigests(new_header);
      if (not babe_digests_res) {
        SL_ERROR(log_,
                 "Could not get digests: {}",
                 babe_digests_res.error().message());
      }

      const auto &[_, babe_header] = babe_digests_res.value();

      EpochDescriptor epoch;
      auto last_known_epoch_res = babe_util_->getLastEpoch();
      if (last_known_epoch_res.has_value()) {
        epoch = prepareFirstEpoch(last_known_epoch_res.value(),
                                  babe_header.slot_number + 1);
        runEpoch(epoch);
        on_synchronized_();
      } else {
        SL_ERROR(log_,
                 "Could not get last known epoch: {}",
                 last_known_epoch_res.error().message());
      }
    }
  }
}  // namespace kagome::consensus::babe
