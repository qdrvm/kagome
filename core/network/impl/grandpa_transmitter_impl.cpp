/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/grandpa_transmitter_impl.hpp"

#include "network/router.hpp"

namespace kagome::network {

  GrandpaTransmitterImpl::GrandpaTransmitterImpl(
      std::shared_ptr<network::Router> router)
      : router_(std::move(router)) {}

  void GrandpaTransmitterImpl::vote(GrandpaVote &&message) {
    auto protocol = router_->getGrandpaProtocol();
    BOOST_ASSERT_MSG(protocol, "Router did not provide grandpa protocol");
    protocol->vote(std::move(message));
  }

  void GrandpaTransmitterImpl::finalize(FullCommitMessage &&message) {
    auto protocol = router_->getGrandpaProtocol();
    BOOST_ASSERT_MSG(protocol, "Router did not provide grandpa protocol");
    protocol->finalize(std::move(message));
  }

  void GrandpaTransmitterImpl::catchUpRequest(const PeerId &peer_id,
                                              CatchUpRequest &&message) {
    auto protocol = router_->getGrandpaProtocol();
    BOOST_ASSERT_MSG(protocol, "Router did not provide grandpa protocol");
    protocol->catchUpRequest(peer_id, std::move(message));
  }

  void GrandpaTransmitterImpl::catchUpResponse(const PeerId &peer_id,
                                               CatchUpResponse &&message) {
    auto protocol = router_->getGrandpaProtocol();
    BOOST_ASSERT_MSG(protocol, "Router did not provide grandpa protocol");
    protocol->catchUpResponse(peer_id, std::move(message));
  }

}  // namespace kagome::network
