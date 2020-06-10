/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/babe_impl.hpp"

#include <sr25519/sr25519.h>
#include <boost/assert.hpp>

#include "blockchain/block_tree_error.hpp"
#include "common/buffer.hpp"
#include "consensus/babe/babe_error.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "consensus/babe/impl/threshold_util.hpp"
#include "consensus/babe/types/babe_block_header.hpp"
#include "consensus/babe/types/seal.hpp"
#include "network/types/block_announce.hpp"
#include "primitives/inherent_data.hpp"
#include "scale/scale.hpp"

namespace kagome::consensus {
  BabeImpl::BabeImpl(
      std::shared_ptr<BabeLottery> lottery,
      std::shared_ptr<BlockExecutor> block_executor,
      std::shared_ptr<storage::trie::TrieStorage> trie_storage,
      std::shared_ptr<EpochStorage> epoch_storage,
      std::shared_ptr<primitives::BabeConfiguration> configuration,
      std::shared_ptr<authorship::Proposer> proposer,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<BabeGossiper> gossiper,
      crypto::SR25519Keypair keypair,
      std::shared_ptr<clock::SystemClock> clock,
      std::shared_ptr<crypto::Hasher> hasher,
      std::unique_ptr<clock::Timer> timer)
      : lottery_{std::move(lottery)},
        block_executor_{std::move(block_executor)},
        trie_storage_{std::move(trie_storage)},
        epoch_storage_{std::move(epoch_storage)},
        genesis_configuration_{std::move(configuration)},
        proposer_{std::move(proposer)},
        block_tree_{std::move(block_tree)},
        gossiper_{std::move(gossiper)},
        keypair_{keypair},
        clock_{std::move(clock)},
        hasher_{std::move(hasher)},
        timer_{std::move(timer)},
        log_{common::createLogger("BABE")} {
    BOOST_ASSERT(lottery_);
    BOOST_ASSERT(epoch_storage_);
    BOOST_ASSERT(trie_storage_);
    BOOST_ASSERT(proposer_);
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(gossiper_);
    BOOST_ASSERT(clock_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(log_);

    NextEpochDescriptor init_epoch_desc;
    init_epoch_desc.randomness = genesis_configuration_->randomness;
    init_epoch_desc.authorities = genesis_configuration_->genesis_authorities;
    [[maybe_unused]] bool init_epoch_desc_ok =
        epoch_storage_->addEpochDescriptor(0, init_epoch_desc).has_value()
        && epoch_storage_->addEpochDescriptor(1, init_epoch_desc).has_value();

    BOOST_ASSERT(init_epoch_desc_ok);
  }

  void BabeImpl::start(ExecutionStrategy strategy) {
    auto &&[best_block_number, best_block_hash] = block_tree_->deepestLeaf();
    log_->debug("Babe run on block with number {} and hash {}",
                best_block_number,
                best_block_hash);

    switch (strategy) {
      case ExecutionStrategy::GENESIS: {
        auto epoch_digest_res = epoch_storage_->getEpochDescriptor(0);
        if (not epoch_digest_res) {
          log_->error("Last epoch digest does not exist for initial epoch");
        }
        auto &&epoch_digest = epoch_digest_res.value();

        auto slot_duration = genesis_configuration_->slot_duration;

        Epoch epoch;
        epoch.epoch_index = 0;
        epoch.authorities = epoch_digest.authorities;
        epoch.randomness = epoch_digest.randomness;
        epoch.epoch_duration = genesis_configuration_->epoch_length;
        // clang-format off
        // TODO(xDimon): remove one of next two variants by decision about way of slot enumeration
        //  variant #1
//      epoch.start_slot = clock_->now().time_since_epoch() / slot_duration;
//      auto starting_slot_finish_time = std::chrono::system_clock::time_point{}
//                                       + slot_duration * epoch.start_slot
//                                       + slot_duration;

        // TODO(xDimon): remove one of next two variants by decision about way of slot enumeration
        //  variant #2
        epoch.start_slot = 0;
        auto starting_slot_finish_time = std::chrono::system_clock::now()
            + slot_duration;
        // clang-format on

        current_state_ = BabeState::SYNCHRONIZED;
        runEpoch(epoch, starting_slot_finish_time);
        break;
      }
      case ExecutionStrategy::SYNC_FIRST: {
        current_state_ = BabeState::WAIT_BLOCK;
        break;
      }
    }
  }

  /**
   * Get index of authority
   *
   * @param authorities list of authorities
   * @param authority_key authority
   * @return index of authority in list of authorities
   */
  boost::optional<uint64_t> getAuthorityIndex(
      const std::vector<primitives::Authority> &authorities,
      const primitives::SessionKey &authority_key) {
    auto it = std::find_if(authorities.begin(),
                           authorities.end(),
                           [&authority_key](const auto &authority) {
                             return authority.id.id == authority_key;
                           });
    if (it == authorities.end()) {
      return boost::none;
    }
    return std::distance(authorities.begin(), it);
  }

  void BabeImpl::runEpoch(Epoch epoch,
                          BabeTimePoint starting_slot_finish_time) {
    BOOST_ASSERT(!epoch.authorities.empty());
    log_->debug("starting an epoch with index {}. Session key: {}",
                epoch.epoch_index,
                keypair_.public_key.toHex());

    current_epoch_ = std::move(epoch);
    current_slot_ = current_epoch_.start_slot;

    slots_leadership_ = getEpochLeadership(current_epoch_);
    next_slot_finish_time_ = starting_slot_finish_time;

    runSlot();
  }

  void BabeImpl::onBlockAnnounce(const network::BlockAnnounce &announce) {
    switch (current_state_) {
      case BabeState::WAIT_BLOCK:
        // TODO(kamilsa): PRE-366 validate block. Now it is problematic as we
        // need t know VRF threshold for validation. To calculate that we need
        // to have weights of the authorities and to get it we need to have
        // the latest state of a blockchain

        // synchronize missing blocks with their bodies
        log_->info("Catching up to block number: {}", announce.header.number);
        current_state_ = BabeState::CATCHING_UP;
        block_executor_->requestBlocks(
            announce.header, [self_weak{weak_from_this()}] {
              if (auto self = self_weak.lock()) {
                self->log_->info("Catching up is done, getting slot time");
                // all blocks were successfully applied, now we need to get
                // slot time
                self->current_state_ = BabeState::NEED_SLOT_TIME;
              }
            });
        break;
      case BabeState::NEED_SLOT_TIME:
        // if block is new add it to the storage and sync missing blocks. Then
        // calculate slot time and execute babe
        block_executor_->processNextBlock(
            announce.header,
            [this](const auto &header) { synchronizeSlots(header); });
        break;
      case BabeState::CATCHING_UP:
      case BabeState::SYNCHRONIZED:
        block_executor_->processNextBlock(announce.header, [](auto &) {});
        break;
    }
  }

  BabeMeta BabeImpl::getBabeMeta() const {
    return BabeMeta{current_epoch_,
                    current_slot_,
                    slots_leadership_,
                    next_slot_finish_time_};
  }

  void BabeImpl::runSlot() {
    bool rewind_slots;
    do {
      if (current_slot_ != 0
          and current_slot_ % genesis_configuration_->epoch_length == 0) {
        // end of the epoch
        finishEpoch();
      }
      log_->info("starting a slot {} in epoch {}",
                 current_slot_,
                 current_epoch_.epoch_index);

      // check that we are really in the middle of the slot, as expected; we can
      // cooperate with a relatively little (kMaxLatency) latency, as our node
      // will be able to retrieve
      auto now = clock_->now();

      rewind_slots = now > next_slot_finish_time_
                     and (now - next_slot_finish_time_)
                             > genesis_configuration_->slot_duration;

      if (rewind_slots) {
        // we are too far behind; after skipping some slots (but not epochs)
        // control will be returned to this method

        current_slot_++;
        next_slot_finish_time_ += genesis_configuration_->slot_duration;
      }
    } while (rewind_slots);

    // everything is OK: wait for the end of the slot
    timer_->expiresAt(next_slot_finish_time_);
    timer_->asyncWait([this](auto &&ec) {
      if (ec) {
        log_->error("error happened while waiting on the timer: {}",
                    ec.message());
        return;
      }
      finishSlot();
    });
  }

  void BabeImpl::finishSlot() {
    auto slot_leadership =
        slots_leadership_[current_slot_ % current_epoch_.epoch_duration];
    if (slot_leadership) {
      log_->debug("Peer {} is leader (vrfOutput: {}, proof: {})",
                  keypair_.public_key.toHex(),
                  common::Buffer(slot_leadership->output).toHex(),
                  common::Buffer(slot_leadership->proof).toHex());

      processSlotLeadership(*slot_leadership);
    }

    ++current_slot_;
    next_slot_finish_time_ += genesis_configuration_->slot_duration;
    log_->debug("Slot {} in epoch {} has finished",
                current_slot_,
                current_epoch_.epoch_index);
    runSlot();
  }

  outcome::result<primitives::PreRuntime> BabeImpl::babePreDigest(
      const crypto::VRFOutput &output,
      primitives::AuthorityIndex authority_index) const {
    BabeBlockHeader babe_header{current_slot_, output, authority_index};
    auto encoded_header_res = scale::encode(babe_header);
    if (!encoded_header_res) {
      log_->error("cannot encode BabeBlockHeader: {}",
                  encoded_header_res.error().message());
      return encoded_header_res.error();
    }
    common::Buffer encoded_header{encoded_header_res.value()};

    return primitives::PreRuntime{{primitives::kBabeEngineId, encoded_header}};
  }

  primitives::Seal BabeImpl::sealBlock(const primitives::Block &block) const {
    auto pre_seal_encoded_block = scale::encode(block.header).value();

    auto pre_seal_hash = hasher_->blake2b_256(pre_seal_encoded_block);

    Seal seal{};
    seal.signature.fill(0);
    sr25519_sign(seal.signature.data(),
                 keypair_.public_key.data(),
                 keypair_.secret_key.data(),
                 pre_seal_hash.data(),
                 decltype(pre_seal_hash)::size());
    auto encoded_seal = common::Buffer(scale::encode(seal).value());
    return primitives::Seal{{primitives::kBabeEngineId, encoded_seal}};
  }

  void BabeImpl::processSlotLeadership(const crypto::VRFOutput &output) {
    // build a block to be announced
    log_->info("Obtained slot leadership");

    primitives::InherentData inherent_data;
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                   clock_->now().time_since_epoch())
                   .count();
    // identifiers are guaranteed to be correct, so use .value() directly
    auto put_res = inherent_data.putData<uint64_t>(kTimestampId, now);
    if (!put_res) {
      return log_->error("cannot put an inherent data: {}",
                         put_res.error().message());
    }
    put_res = inherent_data.putData(kBabeSlotId, current_slot_);
    if (!put_res) {
      return log_->error("cannot put an inherent data: {}",
                         put_res.error().message());
    }

    auto &&[best_block_number, best_block_hash] = block_tree_->deepestLeaf();
    log_->info("Babe builds block on top of block with number {} and hash {}",
               best_block_number,
               best_block_hash);

    auto authority_index_res =
        getAuthorityIndex(current_epoch_.authorities, keypair_.public_key);
    BOOST_ASSERT_MSG(authority_index_res.has_value(), "Authority is not known");
    // calculate babe_pre_digest
    auto babe_pre_digest_res =
        babePreDigest(output, authority_index_res.value());
    if (not babe_pre_digest_res) {
      return log_->error("cannot propose a block: {}",
                         babe_pre_digest_res.error().message());
    }
    auto babe_pre_digest = babe_pre_digest_res.value();

    // create new block
    auto pre_seal_block_res =
        proposer_->propose(best_block_hash, inherent_data, {babe_pre_digest});
    if (!pre_seal_block_res) {
      return log_->error("cannot propose a block: {}",
                         pre_seal_block_res.error().message());
    }

    auto block = pre_seal_block_res.value();

    if (auto next_epoch_digest_res = getNextEpochDigest(block.header);
        next_epoch_digest_res) {
      log_->info("Obtained next epoch digest");
      if (not epoch_storage_->addEpochDescriptor(
              current_epoch_.epoch_index + 2, next_epoch_digest_res.value())) {
        log_->error("Could not add nex epoch digest to epoch storage");
      }
    }

    // seal the block
    auto seal = sealBlock(block);

    // add seal digest item
    block.header.digest.emplace_back(seal);

    // check that we are still in the middle of the
    if (clock_->now()
        > next_slot_finish_time_ + genesis_configuration_->slot_duration) {
      log_->warn(
          "Block was not built in time. Slot has finished. If you are "
          "executing in debug mode, consider to rebuild in release");
      return;
    }
    // add block to the block tree
    if (auto add_res = block_tree_->addBlock(block); not add_res) {
      log_->error("Could not add block: {}", add_res.error().message());
      return;
    }

    auto next_epoch_digest_res = getNextEpochDigest(block.header);
    if (next_epoch_digest_res) {
      log_->info("Got next epoch digest for epoch: {}",
                 current_epoch_.epoch_index + 2);
      if (auto add_epoch_res = epoch_storage_->addEpochDescriptor(
              current_epoch_.epoch_index + 2, next_epoch_digest_res.value());
          not add_epoch_res) {
        log_->error("Could not add next epoch digest. Reason: {}",
                    add_epoch_res.error().message());
      }
    }

    // finally, broadcast the sealed block
    gossiper_->blockAnnounce(network::BlockAnnounce{block.header});
    log_->debug("Announced block number {} in slot {} with timestamp {}",
                block.header.number,
                current_slot_,
                now);
  }

  BabeLottery::SlotsLeadership BabeImpl::getEpochLeadership(
      const Epoch &epoch) const {
    auto authority_index_res =
        getAuthorityIndex(epoch.authorities, keypair_.public_key);
    BOOST_ASSERT_MSG(authority_index_res.has_value(), "Authority is not known");
    auto threshold = calculateThreshold(genesis_configuration_->leadership_rate,
                                        epoch.authorities,
                                        authority_index_res.value());
    return lottery_->slotsLeadership(epoch, threshold, keypair_);
  }

  void BabeImpl::finishEpoch() {
    // compute new randomness
    auto next_epoch_digest_res =
        epoch_storage_->getEpochDescriptor(++current_epoch_.epoch_index);
    if (not next_epoch_digest_res) {
      log_->error("Next epoch digest does not exist for epoch {}",
                  current_epoch_.epoch_index);
      // TODO (kamilsa): uncomment `return;` and remove next line when
      // PRE-364 is done. For now assume no changes after epoch
      //
      // return.
      next_epoch_digest_res =
          NextEpochDescriptor{.authorities = current_epoch_.authorities,
                              .randomness = current_epoch_.randomness};
    }

    current_epoch_.start_slot = current_slot_;
    current_epoch_.authorities = next_epoch_digest_res.value().authorities;
    current_epoch_.randomness = next_epoch_digest_res.value().randomness;
    slots_leadership_ = getEpochLeadership(current_epoch_);

    log_->debug("Epoch {} has finished", current_epoch_.epoch_index);
  }

  void BabeImpl::synchronizeSlots(const primitives::BlockHeader &new_header) {
    static boost::optional<BabeSlotNumber> first_production_slot = boost::none;

    auto babe_digests_res = getBabeDigests(new_header);
    if (not babe_digests_res) {
      log_->error("Could not get digests: {}",
                  babe_digests_res.error().message());
    }

    auto [_, babe_header] = babe_digests_res.value();
    auto observed_slot = babe_header.slot_number;

    if (not first_production_slot) {
      first_production_slot = observed_slot + kSlotTail;
      log_->info("Peer will start produce blocks at slot number: {}",
                 *first_production_slot);
    }

    // get the difference between observed slot and the one that we are trying
    // to launch
    auto diff = *first_production_slot - observed_slot;

    first_slot_times_.emplace_back(
        clock_->now() + diff * genesis_configuration_->slot_duration);
    if (observed_slot >= first_production_slot.value()) {
      current_state_ = BabeState::SYNCHRONIZED;
      log_->info("Slot time obtained. Peer is synchronized");

      // get median as here:
      // https://en.cppreference.com/w/cpp/algorithm/nth_element
      std::nth_element(first_slot_times_.begin(),
                       first_slot_times_.begin() + first_slot_times_.size() / 2,
                       first_slot_times_.end());
      BabeTimePoint first_slot_ending_time =
          first_slot_times_[first_slot_times_.size() / 2];

      Epoch epoch;
      epoch.epoch_index =
          *first_production_slot / genesis_configuration_->epoch_length;
      epoch.start_slot = *first_production_slot;
      epoch.epoch_duration = genesis_configuration_->epoch_length;
      auto next_epoch_digest_res =
          epoch_storage_->getEpochDescriptor(epoch.epoch_index);
      if (not next_epoch_digest_res) {
        log_->error("Could not fetch epoch descriptor for epoch {}. Reason: {}",
                    epoch.epoch_index,
                    next_epoch_digest_res.error().message());
        return;
      }
      epoch.randomness = next_epoch_digest_res.value().randomness;
      epoch.authorities = next_epoch_digest_res.value().authorities;

      runEpoch(epoch, first_slot_ending_time);
    }
  }
}  // namespace kagome::consensus
