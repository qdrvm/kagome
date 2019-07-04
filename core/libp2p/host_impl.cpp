/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/host_impl.hpp"

#include <string_view>

namespace {
  constexpr std::string_view kLibp2pVersion = "ipfs/0.1.0";
  constexpr std::string_view kClientVersion = "cpp-libp2p/0.1.0";
}  // namespace

namespace libp2p {

  HostImpl::HostImpl(Config config, peer::PeerId peer_id)
      : config_{std::move(config)}, id_{std::move(peer_id)} {}

  std::string_view HostImpl::getLibp2pVersion() const noexcept {
    return kLibp2pVersion;
  }

  std::string_view HostImpl::getLibp2pClientVersion() const noexcept {
    return kClientVersion;
  }

  peer::PeerId HostImpl::getId() const noexcept {
    return id_;
  }

  gsl::span<const multi::Multiaddress> HostImpl::getListenAddresses() const {
    return network_->getListenAddresses();
  }

  outcome::result<void> HostImpl::connect(const peer::PeerInfo &p) {
    // TODO(Warchant): review this. this logic may not be here
    auto &&r = network_->dial(p);
    if (!r) {
      return r.error();
    }

    return outcome::success();
  }

  outcome::result<void> HostImpl::newStream(
      const peer::PeerInfo &p, const peer::Protocol &protocol,
      const std::function<connection::Stream::Handler> &handler) {
    return network_->newStream(p, protocol, handler);
  }

  void HostImpl::setProtocolHandler(
      const peer::Protocol &proto,
      const std::function<connection::Stream::Handler> &handler) {
    router_->setProtocolHandler(proto, handler);
  }

  void HostImpl::setProtocolHandler(
      const std::string &prefix,
      const std::function<connection::Stream::Handler> &handler,
      const std::function<bool(const peer::Protocol &)> &predicate) {
    router_->setProtocolHandler(prefix, handler, predicate);
  }

  peer::PeerInfo HostImpl::getPeerInfo() const noexcept {
    return peer::PeerInfo{id_, network_->getListenAddresses()};
  }

  const network::Network &HostImpl::network() const noexcept {
    return *network_;
  }

  peer::PeerRepository &HostImpl::peerRepository() const noexcept {
    return *config_.peer_repository;
  }

  const network::Router &HostImpl::router() const noexcept {
    return *router_;
  }

}  // namespace libp2p
