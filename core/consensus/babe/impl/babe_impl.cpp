/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/babe_impl.hpp"

#include <cmath>

#include <sr25519/sr25519.h>
#include <boost/assert.hpp>
#include "common/buffer.hpp"
#include "consensus/babe/babe_error.hpp"
#include "consensus/babe/types/babe_block_header.hpp"
#include "consensus/babe/types/seal.hpp"
#include "network/types/block_announce.hpp"
#include "primitives/inherent_data.hpp"
#include "scale/scale.hpp"

namespace kagome::consensus {
  BabeImpl::BabeImpl(std::shared_ptr<BabeLottery> lottery,
                     std::shared_ptr<authorship::Proposer> proposer,
                     std::shared_ptr<blockchain::BlockTree> block_tree,
                     std::shared_ptr<network::BabeGossiper> gossiper,
                     crypto::SR25519Keypair keypair,
                     primitives::AuthorityIndex authority_index,
                     std::shared_ptr<clock::SystemClock> clock,
                     std::shared_ptr<crypto::Hasher> hasher,
                     std::unique_ptr<clock::Timer> timer,
                     libp2p::event::Bus &event_bus)
      : lottery_{std::move(lottery)},
        proposer_{std::move(proposer)},
        block_tree_{std::move(block_tree)},
        gossiper_{std::move(gossiper)},
        keypair_{keypair},
        authority_index_{authority_index},
        clock_{std::move(clock)},
        hasher_{std::move(hasher)},
        timer_{std::move(timer)},
        event_bus_{event_bus},
        error_channel_{event_bus_.getChannel<event::BabeErrorChannel>()},
        log_{common::createLogger("BABE")} {
    BOOST_ASSERT(lottery_);
    BOOST_ASSERT(proposer_);
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(gossiper_);
    BOOST_ASSERT(clock_);
    BOOST_ASSERT(hasher_);
  }

  void BabeImpl::runEpoch(Epoch epoch,
                          BabeTimePoint starting_slot_finish_time) {
    BOOST_ASSERT(!epoch.authorities.empty());
    log_->debug("starting an epoch with index {}. Session key: {}",
                epoch.epoch_index,
                keypair_.public_key.toHex());

    current_epoch_ = std::move(epoch);
    current_slot_ = current_epoch_.start_slot;
    slots_leadership_ = lottery_->slotsLeadership(current_epoch_, keypair_);
    next_slot_finish_time_ = starting_slot_finish_time;

    runSlot();
  }

  BabeMeta BabeImpl::getBabeMeta() const {
    return BabeMeta{current_epoch_,
                    current_slot_,
                    slots_leadership_,
                    next_slot_finish_time_};
  }

  void BabeImpl::runSlot() {
    using std::chrono::operator""ms;
    static constexpr auto kMaxLatency = 5000ms;

    if (current_slot_ == current_epoch_.epoch_duration) {
      // end of the epoch
      return finishEpoch();
    }
    log_->info("starting a slot with number {}", current_slot_);

    // check that we are really in the middle of the slot, as expected; we can
    // cooperate with a relatively little (kMaxLatency) latency, as our node
    // will be able to retrieve
    auto now = clock_->now();
    if (now > next_slot_finish_time_
        && (now - next_slot_finish_time_ > kMaxLatency)) {
      // we are too far behind; after skipping some slots (but not epochs)
      // control will be returned to this method
      return synchronizeSlots();
    }

    // everything is OK: wait for the end of the slot
    timer_->expiresAt(next_slot_finish_time_);
    timer_->asyncWait([this](auto &&ec) {
      if (ec) {
        log_->error("error happened while waiting on the timer: {}",
                    ec.message());
        return error_channel_.publish(BabeError::TIMER_ERROR);
      }
      finishSlot();
    });
  }

  void BabeImpl::finishSlot() {
    auto slot_leadership = slots_leadership_[current_slot_];
    if (slot_leadership) {
      log_->debug("Peer {} is leader", authority_index_.index);
      processSlotLeadership(*slot_leadership);
    }

    ++current_slot_;
    next_slot_finish_time_ += current_epoch_.slot_duration;
    log_->debug("Slot {} in epoch {} has finished",
                current_slot_,
                current_epoch_.epoch_index);
    runSlot();
  }

  outcome::result<primitives::PreRuntime> BabeImpl::babePreDigest(
      const crypto::VRFOutput &output) const {
    BabeBlockHeader babe_header{current_slot_, output, authority_index_};
    auto encoded_header_res = scale::encode(babe_header);
    if (!encoded_header_res) {
      log_->error("cannot encode BabeBlockHeader: {}",
                  encoded_header_res.error().message());
      return encoded_header_res.error();
    }
    common::Buffer encoded_header{encoded_header_res.value()};

    return primitives::PreRuntime{{kBabeEngineId, encoded_header}};
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
    return primitives::Seal{{kBabeEngineId, encoded_seal}};
  }

  void BabeImpl::processSlotLeadership(const crypto::VRFOutput &output) {
    // build a block to be announced
    log_->info("Obtained slot leadership");

    primitives::InherentData inherent_data;
    auto epoch_secs = std::chrono::duration_cast<std::chrono::seconds>(
                          clock_->now().time_since_epoch())
                          .count();
    // identifiers are guaranteed to be correct, so use .value() directly
    auto put_res = inherent_data.putData<uint64_t>(kTimestampId, epoch_secs);
    if (!put_res) {
      return log_->error("cannot put an inherent data: {}",
                         put_res.error().message());
    }
    put_res = inherent_data.putData(kBabeSlotId, current_slot_);
    if (!put_res) {
      return log_->error("cannot put an inherent data: {}",
                         put_res.error().message());
    }

    auto &&[_, best_block_hash] = block_tree_->deepestLeaf();

    // calculate babe_pre_digest
    auto babe_pre_digest_res = babePreDigest(output);
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
    // seal the block
    auto seal = sealBlock(block);

    // add seal digest item
    block.header.digest.emplace_back(seal);

    // finally, broadcast the sealed block
    gossiper_->blockAnnounce(network::BlockAnnounce{block.header});
  }

  void BabeImpl::finishEpoch() {
    // TODO(akvinikym) PRE-291: validator update - how is it done?

    // TODO(akvinikym) PRE-291: compute new epoch duration
    // (BabeApi_slot_duration runtime entry)

    // TODO(akvinikym) PRE-291: compute new threshold
    // (BabeApi_slot_winning_threshold runtime entry)

    // compute new randomness
    current_epoch_.randomness = lottery_->computeRandomness(
        current_epoch_.randomness, ++current_epoch_.epoch_index);
    current_epoch_.start_slot = 0;

    log_->debug("Epoch {} has finished", current_epoch_.epoch_index);
    runEpoch(current_epoch_, next_slot_finish_time_);
  }

  void BabeImpl::synchronizeSlots() {
    // assuming we are too far behind other peers (now() >> finishing time of
    // the last known slot), we compute, how much slots we should skip; if this
    // would require us to move through the epochs, it becomes problematic, as
    // many things are to be updated on the epochs' borders, and we don't have
    // enough information; give up in that case
    log_->info("trying to synchronize our node with the others");

    auto delay_in_time = clock_->now() - next_slot_finish_time_;

    // result of the division can be both integer or float number; in both
    // cases, we want that number to be incremented by one, so that after the
    // sync the time would point on the finishing time of the next slot
    auto delay_in_slots =
        std::floor(delay_in_time / current_epoch_.slot_duration);
    ++delay_in_slots;

    if (current_slot_ + delay_in_slots >= current_epoch_.epoch_duration) {
      log_->error("the node is too far behind");
      return error_channel_.publish(BabeError::NODE_FALL_BEHIND);
    }

    // everything is OK: skip slots, set a new time and return control
    current_slot_ += delay_in_slots - 1;
    next_slot_finish_time_ += std::chrono::milliseconds(
        static_cast<uint64_t>(delay_in_slots) * current_epoch_.slot_duration);
    runSlot();
  }
}  // namespace kagome::consensus
