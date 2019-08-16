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
#include "common/logger.hpp"
#include "consensus/babe.hpp"
#include "consensus/babe/babe_lottery.hpp"
#include "crypto/crypto_types.hpp"
#include "crypto/hasher.hpp"
#include "libp2p/event/bus.hpp"
#include "network/network_state.hpp"
#include "primitives/common.hpp"

namespace kagome::consensus {
  namespace event {
    /// channel, over which critical errors from BABE are emitted; after such
    /// errors block production stops
    struct BabeError {};
    using BabeErrorChannel =
        libp2p::event::channel_decl<BabeError, std::error_code>;
  }  // namespace event

  class BabeImpl : public Babe, public std::enable_shared_from_this<BabeImpl> {
    using Timer = boost::asio::basic_waitable_timer<std::chrono::system_clock>;

   public:
    /**
     * Create an instance of Babe implementation
     * @param lottery - implementation of Babe Lottery
     * @param proposer - block proposer
     * @param block_tree - tree of the blocks
     * @param network of this consensus
     * @param keypair - SR25519 keypair of this node
     * @param authority_id of this node
     * @param clock to measure time
     * @param hasher to take hashes
     * @param timer to be used by the implementation
     * @param event_bus to deliver events over
     * @param log to write messages to
     */
    BabeImpl(std::shared_ptr<BabeLottery> lottery,
             std::shared_ptr<authorship::Proposer> proposer,
             std::shared_ptr<blockchain::BlockTree> block_tree,
             std::shared_ptr<network::NetworkState> network,
             crypto::SR25519Keypair keypair,
             primitives::AuthorityIndex authority_id,
             std::shared_ptr<clock::SystemClock> clock,
             std::shared_ptr<crypto::Hasher> hasher,
             Timer timer,
             libp2p::event::Bus &event_bus,
             common::Logger log = common::createLogger("BABE"));

    ~BabeImpl() override = default;

    void runEpoch(Epoch epoch,
                  BabeTimePoint starting_slot_finish_time) override;

    BabeMeta getBabeMeta() const override;

    enum class Error { TIMER_ERROR = 1, NODE_FALL_BEHIND };

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
     * @param proof that we are the leader of this slot
     */
    void processSlotLeadership(const crypto::VRFOutput &proof);

    /**
     * Finish the Babe epoch
     */
    void finishEpoch();

    /**
     * To be called if we are far behind other nodes to skip some slots and
     * finally synchronize with the network
     */
    void synchronizeSlots();

    std::shared_ptr<BabeLottery> lottery_;
    std::shared_ptr<authorship::Proposer> proposer_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<network::NetworkState> network_;
    crypto::SR25519Keypair keypair_;
    primitives::AuthorityIndex authority_id_;
    std::shared_ptr<clock::SystemClock> clock_;
    std::shared_ptr<crypto::Hasher> hasher_;
    Timer timer_;
    libp2p::event::Bus &event_bus_;
    common::Logger log_;

    Epoch current_epoch_;

    BabeSlotNumber current_slot_{};
    BabeLottery::SlotsLeadership slots_leadership_;
    BabeTimePoint next_slot_finish_time_;

    decltype(event_bus_.getChannel<event::BabeErrorChannel>()) &error_channel_;
  };
}  // namespace kagome::consensus

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus, BabeImpl::Error)

#endif  // KAGOME_BABE_IMPL_HPP
