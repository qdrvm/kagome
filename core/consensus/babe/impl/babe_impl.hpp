/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BABE_IMPL_HPP
#define KAGOME_BABE_IMPL_HPP

#include <memory>

#include <boost/asio/basic_waitable_timer.hpp>
#include <outcome/outcome.hpp>
#include "authorship/proposer.hpp"
#include "blockchain/block_tree.hpp"
#include "clock/timer.hpp"
#include "common/logger.hpp"
#include "consensus/babe.hpp"
#include "consensus/babe/babe_lottery.hpp"
#include "crypto/hasher.hpp"
#include "crypto/sr25519_types.hpp"
#include "libp2p/event/bus.hpp"
#include "network/babe_gossiper.hpp"
#include "primitives/common.hpp"

namespace kagome::consensus {
  inline const auto kBabeEngineId =
      primitives::ConsensusEngineId::fromString("BABE").value();
  inline const auto kTimestampId =
      primitives::InherentIdentifier::fromString("timstap0").value();
  inline const auto kBabeSlotId =
      primitives::InherentIdentifier::fromString("babeslot").value();

  namespace event {
    /// channel, over which critical errors from BABE are emitted; after such
    /// errors block production stops
    struct BabeError {};
    using BabeErrorChannel =
        libp2p::event::channel_decl<BabeError, std::error_code>;
  }  // namespace event

  class BabeImpl : public Babe, public std::enable_shared_from_this<BabeImpl> {
   public:
    /**
     * Create an instance of Babe implementation
     * @param lottery - implementation of Babe Lottery
     * @param proposer - block proposer
     * @param block_tree - tree of the blocks
     * @param gossiper of this consensus
     * @param keypair - SR25519 keypair of this node
     * @param authority_index of this node
     * @param clock to measure time
     * @param hasher to take hashes
     * @param timer to be used by the implementation; the recommended one is
     * kagome::clock::BasicWaitableTimer
     * @param event_bus to deliver events over
     */
    BabeImpl(std::shared_ptr<BabeLottery> lottery,
             std::shared_ptr<authorship::Proposer> proposer,
             std::shared_ptr<blockchain::BlockTree> block_tree,
             std::shared_ptr<network::BabeGossiper> gossiper,
             crypto::SR25519Keypair keypair,
             primitives::AuthorityIndex authority_index,
             std::shared_ptr<clock::SystemClock> clock,
             std::shared_ptr<crypto::Hasher> hasher,
             std::unique_ptr<clock::Timer> timer,
             libp2p::event::Bus &event_bus);

    ~BabeImpl() override = default;

    void runEpoch(Epoch epoch,
                  BabeTimePoint starting_slot_finish_time) override;

    BabeMeta getBabeMeta() const override;

   private:
    /**
     * Run the next Babe slot
     */
    void runSlot();

    /**
     * Finish the current Babe slot
     */
    void finishSlot();

    /**
     * Gather the block and broadcast it
     * @param output that we are the leader of this slot
     */
    void processSlotLeadership(const crypto::VRFOutput &output);

    /**
     * Finish the Babe epoch
     */
    void finishEpoch();

    /**
     * To be called if we are far behind other nodes to skip some slots and
     * finally synchronize with the network
     */
    void synchronizeSlots();

   private:
    std::shared_ptr<BabeLottery> lottery_;
    std::shared_ptr<authorship::Proposer> proposer_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<network::BabeGossiper> gossiper_;
    crypto::SR25519Keypair keypair_;
    primitives::AuthorityIndex authority_index_;
    std::shared_ptr<clock::SystemClock> clock_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::unique_ptr<clock::Timer> timer_;
    libp2p::event::Bus &event_bus_;

    Epoch current_epoch_;

    BabeSlotNumber current_slot_{};
    BabeLottery::SlotsLeadership slots_leadership_;
    BabeTimePoint next_slot_finish_time_;

    decltype(event_bus_.getChannel<event::BabeErrorChannel>()) &error_channel_;
    common::Logger log_;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_BABE_IMPL_HPP
