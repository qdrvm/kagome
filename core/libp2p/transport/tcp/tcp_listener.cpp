/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/transport/tcp/tcp_listener.hpp"

namespace libp2p::transport {

  TcpListener::TcpListener(boost::asio::io_context &context,
                           std::shared_ptr<Upgrader> upgrader,
                           TransportListener::HandlerFunc handler)
      : context_(context),
        acceptor_(context),
        upgrader_(std::move(upgrader)),
        handle_(std::move(handler)) {}

  outcome::result<void> TcpListener::listen(
      const multi::Multiaddress &address) {
    if (!canListen(address)) {
      return std::errc::address_family_not_supported;
    }

    if (acceptor_.is_open()) {
      return std::errc::already_connected;
    }

    // TODO(@warchant): replace with parser PRE-129
    using namespace boost::asio;  // NOLINT
    try {
      OUTCOME_TRY(endpoint, detail::makeEndpoint(address));

      // setup acceptor, throws
      acceptor_.open(endpoint.protocol());
      acceptor_.set_option(ip::tcp::acceptor::reuse_address(true));
      acceptor_.bind(endpoint);
      acceptor_.listen();

      // start listening
      do_accept();

      return outcome::success();
    } catch (const boost::system::system_error &e) {
      return e.code();
    }
  }

  bool TcpListener::canListen(const multi::Multiaddress &ma) const {
    return detail::supportsIpTcp(ma);
  }

  outcome::result<multi::Multiaddress> TcpListener::getListenMultiaddr() const {
    return detail::makeEndpoint(acceptor_.local_endpoint());
  }

  bool TcpListener::isClosed() const {
    return !acceptor_.is_open();
  }

  outcome::result<void> TcpListener::close() {
    boost::system::error_code ec;
    acceptor_.close(ec);
    if (ec) {
      return outcome::failure(ec);
    }

    return outcome::success();
  }

  void TcpListener::do_accept() {
    using namespace boost::asio;    // NOLINT
    using namespace boost::system;  // NOLINT

    if (!acceptor_.is_open()) {
      return;
    }

    acceptor_.async_accept([self{this->shared_from_this()}](
                               const boost::system::error_code &ec,
                               ip::tcp::socket sock) {
      if (ec) {
        return self->handle_(ec);
      }

      ufiber::spawn(
          self->context_,
          [sock{std::move(sock)}, self](basic::yield_t yield) mutable {
            auto conn = std::make_shared<TcpConnection>(yield, std::move(sock));

            // upgrade to secure
            auto rsc = self->upgrader_->upgradeToSecure(std::move(conn));
            if (!rsc) {
              return self->handle_(rsc.error());
            }

            // upgrade to muxed
            auto rmc = self->upgrader_->upgradeToMuxed(std::move(rsc.value()));
            if (!rmc) {
              return self->handle_(rmc.error());
            }

            // execute user-provided callback
            return self->handle_(std::move(rmc.value()));
          });

      self->do_accept();
    });
  }
}  // namespace libp2p::transport
