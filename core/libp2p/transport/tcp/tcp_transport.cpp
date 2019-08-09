/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/transport/tcp/tcp_transport.hpp"

#include "libp2p/transport/impl/upgrader_session.hpp"

namespace libp2p::transport {

  void TcpTransport::dial(const peer::PeerId &remoteId,
                          multi::Multiaddress address,
                          TransportAdaptor::HandlerFunc handler) {
    if (!canDial(address)) {
      return handler(std::errc::address_family_not_supported);
    }

    auto conn = std::make_shared<TcpConnection>(context_);
    auto rendpoint = detail::makeEndpoint(address);
    if (!rendpoint) {
      return handler(rendpoint.error());
    }

    conn->resolve(rendpoint.value(),
                  [self{shared_from_this()}, conn, handler{std::move(handler)},
                   remoteId](auto ec, auto r) mutable {
                    if (ec) {
                      return handler(ec);
                    }

                    conn->connect(
                        r,
                        [self, conn, handler{std::move(handler)}, remoteId](
                            auto ec, auto &e) mutable {
                          if (ec) {
                            return handler(ec);
                          }

                          auto session = std::make_shared<UpgraderSession>(
                              self->upgrader_, std::move(conn), handler);

                          session->secureOutbound(remoteId);
                        });
                  });
  }

  std::shared_ptr<TransportListener> TcpTransport::createListener(
      TransportListener::HandlerFunc handler) {
    return std::make_shared<TcpListener>(context_, upgrader_,
                                         std::move(handler));
  }

  bool TcpTransport::canDial(const multi::Multiaddress &ma) const {
    return detail::supportsIpTcp(ma);
  }

  TcpTransport::TcpTransport(boost::asio::io_context &context,
                             std::shared_ptr<Upgrader> upgrader)
      : context_(context), upgrader_(std::move(upgrader)) {}

  peer::Protocol TcpTransport::getProtocolId() const {
    return "/tcp/1.0.0";
  }

}  // namespace libp2p::transport
