/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/host.hpp"

#include <string_view>

namespace {
  constexpr std::string_view kLibp2pVersion = "ipfs/0.1.0";
  constexpr std::string_view kClientVersion = "cpp-libp2p/0.1.0";
}  // namespace

namespace libp2p {

  Host::Host(Config config, peer::PeerId peer_id)
      : config_{std::move(config)}, id_{std::move(peer_id)} {}

  std::string_view Host::getLibp2pVersion() const noexcept {
    return kLibp2pVersion;
  }

  std::string_view Host::getLibp2pClientVersion() const noexcept {
    return kClientVersion;
  }

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

  void Host::setProtocolHandler(
      const std::string &prefix,
      const std::function<connection::Stream::Handler> &handler,
      const std::function<bool(const peer::Protocol &)> &predicate) {
    router_->setProtocolHandler(prefix, handler, predicate);
  }

  peer::PeerInfo Host::getPeerInfo() const noexcept {
    return peer::PeerInfo{id_, network_->getListenAddresses()};
  }

  const network::Router &Host::router() const noexcept {
    return *router_;
  }

}  // namespace libp2p
