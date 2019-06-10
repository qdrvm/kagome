/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/transport/tcp/tcp_transport.hpp"

namespace libp2p::transport {

  void TcpTransport::dial(const multi::Multiaddress &address,
                          Transport::HandlerFunc handler) const {
    ufiber::spawn(
        context_,
        [address, handler_{std::move(handler)},
         self{this->shared_from_this()}](basic::yield_t yield) mutable {
          if (!self->canDial(address)) {
            return handler_(std::errc::address_family_not_supported);
          }

          auto conn = std::make_shared<TcpConnection>(yield);
          auto rendpoint = detail::makeEndpoint(address);
          if (!rendpoint) {
            return handler_(rendpoint.error());
          }

          // connect to the other peer
          if (auto r = conn->connect(rendpoint.value()); !r) {
            return handler_(r.error());
          }

          // upgrade to secure
          auto rsc = self->upgrader_->upgradeToSecure(std::move(conn));
          if (!rsc) {
            return handler_(rsc.error());
          }

          // upgrade to muxed
          auto rmc = self->upgrader_->upgradeToMuxed(std::move(rsc.value()));
          if (!rmc) {
            return handler_(rmc.error());
          }

          // execute handler with value
          return handler_(std::move(rmc.value()));
        });
  }  // namespace libp2p::transport

  std::shared_ptr<TransportListener> TcpTransport::createListener(
      TransportListener::HandlerFunc handler) const {
    return std::make_shared<TcpListener>(context_, upgrader_,
                                         std::move(handler));
  }

  bool TcpTransport::canDial(const multi::Multiaddress &ma) const {
    return detail::supportsIpTcp(ma);
  }

  TcpTransport::TcpTransport(boost::asio::io_context &context,
                             std::shared_ptr<Upgrader> upgrader)
      : context_(context), upgrader_(std::move(upgrader)) {}
}  // namespace libp2p::transport
