/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/host.hpp"

namespace libp2p {

  Host::Host(Config config, peer::PeerId peer_id)
      : config_{std::move(config)}, id_{std::move(peer_id)} {}

  peer::PeerId Host::getId() const noexcept {
    return id_;
  }

  gsl::span<const multi::Multiaddress> Host::getListenAddresses() const {
    return network_->getListenAddresses();
  }

  outcome::result<void> Host::connect(const peer::PeerInfo &p) {
    // TODO(Warchant): review this. this logic may not be here
    auto &&r = network_->dial(p);
    if (!r) {
      return r.error();
    }

    return outcome::success();
  }

  outcome::result<void> Host::newStream(
      const peer::PeerInfo &p, const peer::Protocol &protocol,
      const std::function<connection::Stream::Handler> &handler) {
    return network_->newStream(p, protocol, handler);
  }

  void Host::setProtocolHandler(
      const peer::Protocol &proto,
      const std::function<connection::Stream::Handler> &handler) {
    router_->setProtocolHandler(proto, handler);
  }

  void Host::setProtocolHandlerByPrefix(
      std::string_view prefix,
      const std::function<bool(const peer::Protocol &)> &predicate,
      const std::function<connection::Stream::Handler> &handler) {
    router_->setProtocolHandler(std::string{prefix}, handler, predicate);
  }

  peer::PeerInfo Host::getPeerInfo() const noexcept {
    return peer::PeerInfo{id_, network_->getListenAddresses()};
  }

}  // namespace libp2p
