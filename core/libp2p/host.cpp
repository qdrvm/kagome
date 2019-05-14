/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/host.hpp"

namespace libp2p {

  peer::PeerId Node::getId() const noexcept {
    return id_;
  }

  gsl::span<const multi::Multiaddress> Node::getListenAddresses() {
    // TODO
  }

  peer::PeerRepository &Node::getPeerRepository() const {
    return *config_.peer_repository;
  }

  outcome::result<void> Node::addKnownPeer(
      const peer::PeerId &p, gsl::span<const multi::Multiaddress> mas,
      std::chrono::milliseconds ttl) {
    return config_.peer_repository->getAddressRepository().upsertAddresses(
        p, mas, ttl);
  }

  outcome::result<void> Node::newStream(
      const peer::PeerId &p, const peer::Protocol &protocol,
      const std::function<void(std::unique_ptr<stream::Stream>)> &handler) {
    // TODO(@warchant):
    // 1. if upgraded connection to p exists, then re-use it and do
    // muxer.newStream()
    // 2. if no upgraded connection to p exist, and we know multiaddress of p
    // (via peer repository), then connect + upgrade, then muxer.newStream()
    // 3. if no peer p in peer repository, return error "unknown peer
    // address"

    return outcome::success();
  }

  void Node::setProtocolHandler(
      const peer::Protocol &proto,
      const std::function<void(std::unique_ptr<stream::Stream>)> &handler) {
    // TODO
  }

}  // namespace libp2p
