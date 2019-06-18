/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/transport/tcp/tcp_transport.hpp"

namespace libp2p::transport {

  void TcpTransport::dial(multi::Multiaddress address,
                          Transport::HandlerFunc handler) {
    if (!canDial(address)) {
      return handler(std::errc::address_family_not_supported);
    }

    auto conn = std::make_shared<TcpConnection>(context_);
    auto rendpoint = detail::makeEndpoint(address);
    if (!rendpoint) {
      return handler(rendpoint.error());
    }

    conn->resolve(rendpoint.value(),
                  [self{this->shared_from_this()}, conn,
                   handler{std::move(handler)}](auto ec, auto r) mutable {
                    if (ec) {
                      return handler(ec);
                    }

                    self->onResolved(std::move(conn), std::move(r),
                                     std::move(handler));
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

  void TcpTransport::onResolved(std::shared_ptr<TcpConnection> c,
                                const TcpConnection::ResolverResultsType &r,
                                Transport::HandlerFunc handler) {
    c->connect(r,
               [self{this->shared_from_this()}, c, handler{std::move(handler)}](
                   auto ec, auto &e) mutable {
                 if (ec) {
                   return handler(ec);
                 }

                 self->onConnected(std::move(c), std::move(handler));
               });
  }

  void TcpTransport::onConnected(std::shared_ptr<TcpConnection> c,
                                 Transport::HandlerFunc handler) {
    this->upgrader_->upgradeToSecure(
        std::move(c),
        [self{shared_from_this()},
         handler{std::move(handler)}](auto &&rsecureConn) mutable {
          if (!rsecureConn) {
            return handler(rsecureConn.error());
          }

          return self->upgrader_->upgradeToMuxed(
              std::move(rsecureConn.value()),
              [self, handler{std::move(handler)}](auto &&rcapableConn) mutable {
                // handles both error and value
                return handler(
                    std::forward<decltype(rcapableConn)>(rcapableConn));
              });
        });
  }

}  // namespace libp2p::transport
