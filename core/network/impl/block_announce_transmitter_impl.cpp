/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/block_announce_transmitter_impl.hpp"

#include "network/router.hpp"

namespace kagome::network {

  BlockAnnounceTransmitterImpl::BlockAnnounceTransmitterImpl(
      std::shared_ptr<network::Router> router)
      : router_(std::move(router)) {}

  void BlockAnnounceTransmitterImpl::blockAnnounce(BlockAnnounce &&message) {
    auto protocol = router_->getBlockAnnounceProtocol();
    BOOST_ASSERT_MSG(protocol,
                     "Router did not provide block announce protocol");
    protocol->blockAnnounce(std::move(message));
  }
}  // namespace kagome::network
