#include <utility>

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/transport/tcp/tcp_server.hpp"

namespace libp2p::transport {

  asio::ServerFactory::server_ptr_result TcpServer::create(
      boost::asio::io_context &context, const Tcp::endpoint &endpoint,
      HandlerFunc handler) {
    try {
      auto s = std::unique_ptr<TcpServer>(
          new TcpServer(context, std::move(handler)));

      // setup acceptor
      s->acceptor_.open(endpoint.protocol());
      s->acceptor_.set_option(Tcp::acceptor::reuse_address(true));
      s->acceptor_.non_blocking(true);
      s->acceptor_.bind(endpoint);
      s->acceptor_.listen();
      return s;
    } catch (const boost::system::system_error &e) {
      return e.code();
    }
  }

  TcpServer::TcpServer(boost::asio::io_context &context, HandlerFunc handler)
      : context_(context), acceptor_(context), handler_(std::move(handler)) {}

  bool TcpServer::isClosed() const {
    return !acceptor_.is_open();
  }

  outcome::result<void> TcpServer::close() {
    boost::system::error_code ec{};
    acceptor_.close(ec);
    if (ec) {
      return outcome::failure(ec);
    }

    return outcome::success();
  }

  void TcpServer::startAccept() {
    if (!acceptor_.is_open()) {
      return;  // return early
    }

    acceptor_.async_accept(
        context_, [this](const boost::system::error_code &ec, Socket socket) {
          if (!ec) {
            this->handler_(std::move(socket));
          } else {
            this->handler_(ec);
          }

          startAccept();
        });
  }

  multi::Multiaddress TcpServer::getMultiaddress() const {
    auto le = acceptor_.local_endpoint();
    auto address = le.address();
    auto port = le.port();

    std::ostringstream s;
    if (address.is_v4()) {
      s << "/ip4/" << address.to_v4().to_string();
    } else {
      s << "/ip6/" << address.to_v6().to_string();
    }

    s << "/tcp/" << port;

    return multi::Multiaddress::create(s.str()).value();
  }

}  // namespace libp2p::transport
