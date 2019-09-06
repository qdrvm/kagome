/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/babe_impl.hpp"

#include <cmath>

#include <sr25519/sr25519.h>
#include <boost/assert.hpp>
#include "common/buffer.hpp"
#include "consensus/babe/types/babe_block_header.hpp"
#include "consensus/babe/types/seal.hpp"
#include "network/types/block_announce.hpp"
#include "primitives/inherent_data.hpp"
#include "scale/scale.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus, BabeImpl::Error, e) {
  using E = kagome::consensus::BabeImpl::Error;
  switch (e) {
    case E::TIMER_ERROR:
      return "some internal error happened while using the timer in BABE; "
             "please, see logs";
    case E::NODE_FALL_BEHIND:
      return "local node has fallen too far behind the others, most likely it "
             "is in one of the previous epochs";
  }
  return "unknown error";
}

namespace kagome::consensus {
  BabeImpl::BabeImpl(std::shared_ptr<BabeLottery> lottery,
                     std::shared_ptr<authorship::Proposer> proposer,
                     std::shared_ptr<blockchain::BlockTree> block_tree,
                     std::shared_ptr<network::BabeGossiper> gossiper,
                     crypto::SR25519Keypair keypair,
                     primitives::AuthorityIndex authority_id,
                     std::shared_ptr<clock::SystemClock> clock,
                     std::shared_ptr<crypto::Hasher> hasher,
                     Timer timer,
                     libp2p::event::Bus &event_bus,
                     common::Logger log)
      : lottery_{std::move(lottery)},
        proposer_{std::move(proposer)},
        block_tree_{std::move(block_tree)},
        gossiper_{std::move(gossiper)},
        keypair_{keypair},
        authority_id_{authority_id},
        clock_{std::move(clock)},
        hasher_{std::move(hasher)},
        timer_{std::move(timer)},
        event_bus_{event_bus},
        log_{std::move(log)},
        error_channel_{event_bus_.getChannel<event::BabeErrorChannel>()} {
    BOOST_ASSERT(lottery_);
    BOOST_ASSERT(proposer_);
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(gossiper_);
    BOOST_ASSERT(clock_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(log_);
  }

  void BabeImpl::runEpoch(Epoch epoch,
                          BabeTimePoint starting_slot_finish_time) {
    BOOST_ASSERT(!epoch.authorities.empty());
    log_->info("starting an epoch with index {}", epoch.epoch_index);

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
    timer_.expires_at(next_slot_finish_time_);
    timer_.async_wait([this](auto &&ec) {
      if (ec) {
        log_->error("error happened while waiting on the timer: {}",
                    ec.message());
        return error_channel_.publish(Error::TIMER_ERROR);
      }
      finishSlot();
    });
  }

  void BabeImpl::finishSlot() {
    auto slot_leadership = slots_leadership_[current_slot_];
    if (slot_leadership) {
      processSlotLeadership(*slot_leadership);
    }

    ++current_slot_;
    next_slot_finish_time_ += current_epoch_.slot_duration;
    runSlot();
  }

  void BabeImpl::processSlotLeadership(const crypto::VRFOutput &output) {
    // build a block to be announced
    auto &&[_, best_block_hash] = block_tree_->deepestLeaf();

    BabeBlockHeader babe_header{current_slot_, output, authority_id_};
    auto encoded_header_res = scale::encode(babe_header);
    if (!encoded_header_res) {
      return log_->error("cannot encode BabeBlockHeader: {}",
                         encoded_header_res.error().message());
    }

    primitives::InherentData inherent_data;
    auto epoch_secs = std::chrono::duration_cast<std::chrono::seconds>(
                          clock_->now().time_since_epoch())
                          .count();
    // identifiers are guaranteed to be correct, so use .value() directly
    auto put_res = inherent_data.putData(
        primitives::InherentIdentifier::fromString("timstap0").value(),
        common::Buffer{}.putUint64(epoch_secs));
    if (!put_res) {
      return log_->error("cannot put an inherent data: {}",
                         put_res.error().message());
    }
    put_res = inherent_data.putData(
        primitives::InherentIdentifier::fromString("babeslot").value(),
        common::Buffer{}.putUint64(current_slot_));
    if (!put_res) {
      return log_->error("cannot put an inherent data: {}",
                         put_res.error().message());
    }

    auto pre_seal_block_res = proposer_->propose(
        best_block_hash,
        inherent_data,
        {common::Buffer{std::move(encoded_header_res.value())}});
    if (!pre_seal_block_res) {
      return log_->error("cannot propose a block: {}",
                         pre_seal_block_res.error().message());
    }

    // seal the block
    auto block = std::move(pre_seal_block_res.value());
    auto pre_seal_encoded_block_res = scale::encode(block);
    if (!pre_seal_encoded_block_res) {
      return log_->error("cannot encode pre-seal block: {}",
                         pre_seal_encoded_block_res.error().message());
    }
    auto pre_seal_hash =
        hasher_->blake2b_256(pre_seal_encoded_block_res.value());

    Seal seal{};
    seal.signature.fill(0);
    sr25519_sign(seal.signature.data(),
                 keypair_.public_key.data(),
                 keypair_.secret_key.data(),
                 pre_seal_hash.data(),
                 decltype(pre_seal_hash)::size());
    auto encoded_seal_res = scale::encode(seal);
    if (!encoded_seal_res) {
      return log_->error("cannot encoded seal: {}",
                         encoded_seal_res.error().message());
    }

    block.header.digests.emplace_back(
        common::Buffer{std::move(encoded_seal_res.value())});

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

    runEpoch(current_epoch_, next_slot_finish_time_);
  }

  void BabeImpl::synchronizeSlots() {
    // assuming we are too far behind other peers (now() > finishing time of the
    // last known slot), we compute, how much slots we should skip; if this
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
      return error_channel_.publish(Error::NODE_FALL_BEHIND);
    }

    // everything is OK: skip slots, set a new time and return control
    current_slot_ += delay_in_slots - 1;
    next_slot_finish_time_ += std::chrono::milliseconds(
        static_cast<uint64_t>(delay_in_slots) * current_epoch_.slot_duration);
    runSlot();
  }
}  // namespace kagome::consensus
