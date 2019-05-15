/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/host.hpp"

namespace libp2p {

  peer::PeerId Host::getId() const noexcept {
    return id_;
  }

  gsl::span<const multi::Multiaddress> Host::getListenAddresses() {
    return connection_manager_->getListenAddresses();
  }

  peer::PeerRepository &Host::getPeerRepository() const {
    return *config_.peer_repository;
  }

  outcome::result<void> Host::addKnownPeer(
      const peer::PeerId &p, gsl::span<const multi::Multiaddress> mas,
      std::chrono::milliseconds ttl) {
    return config_.peer_repository->getAddressRepository().upsertAddresses(
        p, mas, ttl);
  }

  outcome::result<void> Host::connect(const peer::PeerInfo &p) {
    // TODO: review this. this logic may not be here
    auto &&r = connection_manager_->dial(p);
    if (!r) {
      return r.error();
    }

    return outcome::success();
  }

  outcome::result<void> Host::newStream(
      const peer::PeerInfo &p, const peer::Protocol &protocol,
      const std::function<stream::Stream::Handler> &handler) {
    return switch_->newStream(p, protocol, handler);
  }

  void Host::setProtocolHandler(
      const peer::Protocol &proto,
      const std::function<stream::Stream::Handler> &handler) {
    protocol_manager_->setProtocolHandler(proto, handler);
  }

  void Host::setProtocolHandlerMatch(
      std::string_view prefix,
      const std::function<bool(const peer::Protocol &)> &predicate,
      const std::function<stream::Stream::Handler> &handler) {
    protocol_manager_->setProtocolHandlerMatch(prefix, predicate, handler);
  }

}  // namespace libp2p
