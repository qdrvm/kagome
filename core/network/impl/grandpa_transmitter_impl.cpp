/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/grandpa_transmitter_impl.hpp"

#include "network/impl/protocols/grandpa_protocol.hpp"
#include "network/router.hpp"

namespace kagome::network {

  GrandpaTransmitterImpl::GrandpaTransmitterImpl(
      std::shared_ptr<network::Router> router)
      : router_(std::move(router)) {}

  void GrandpaTransmitterImpl::sendNeighborMessage(
      GrandpaNeighborMessage &&message) {
    auto protocol = router_->getGrandpaProtocol();
    BOOST_ASSERT_MSG(protocol, "Router did not provide grandpa protocol");
    // NOLINTNEXTLINE(hicpp-move-const-arg,performance-move-const-arg)
    protocol->neighbor(std::move(message));
  }

  void GrandpaTransmitterImpl::sendVoteMessage(
      const libp2p::peer::PeerId &peer_id, GrandpaVote &&message) {
    auto protocol = router_->getGrandpaProtocol();
    BOOST_ASSERT_MSG(protocol, "Router did not provide grandpa protocol");
    protocol->vote(std::move(message), peer_id);
  }

  void GrandpaTransmitterImpl::sendVoteMessage(GrandpaVote &&message) {
    auto protocol = router_->getGrandpaProtocol();
    BOOST_ASSERT_MSG(protocol, "Router did not provide grandpa protocol");
    protocol->vote(std::move(message), std::nullopt);
  }

  void GrandpaTransmitterImpl::sendCommitMessage(
      const libp2p::peer::PeerId &peer_id, FullCommitMessage &&message) {
    auto protocol = router_->getGrandpaProtocol();
    BOOST_ASSERT_MSG(protocol, "Router did not provide grandpa protocol");
    protocol->finalize(std::move(message), peer_id);
  }

  void GrandpaTransmitterImpl::sendCommitMessage(FullCommitMessage &&message) {
    auto protocol = router_->getGrandpaProtocol();
    BOOST_ASSERT_MSG(protocol, "Router did not provide grandpa protocol");
    protocol->finalize(std::move(message), std::nullopt);
  }

  void GrandpaTransmitterImpl::sendCatchUpRequest(const PeerId &peer_id,
                                                  CatchUpRequest &&message) {
    auto protocol = router_->getGrandpaProtocol();
    BOOST_ASSERT_MSG(protocol, "Router did not provide grandpa protocol");
    // NOLINTNEXTLINE(hicpp-move-const-arg,performance-move-const-arg)
    protocol->catchUpRequest(peer_id, std::move(message));
  }

  void GrandpaTransmitterImpl::sendCatchUpResponse(const PeerId &peer_id,
                                                   CatchUpResponse &&message) {
    auto protocol = router_->getGrandpaProtocol();
    BOOST_ASSERT_MSG(protocol, "Router did not provide grandpa protocol");
    protocol->catchUpResponse(peer_id, std::move(message));
  }

}  // namespace kagome::network
