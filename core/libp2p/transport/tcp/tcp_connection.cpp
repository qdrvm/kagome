/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/transport/tcp/tcp_connection.hpp"

#include "libp2p/transport/tcp/tcp_util.hpp"

namespace libp2p::transport {

  TcpConnection::TcpConnection(boost::asio::io_context &ctx,
                               boost::asio::ip::tcp::socket &&socket)
      : context_(ctx), socket_(std::move(socket)) {}

  TcpConnection::TcpConnection(boost::asio::io_context &ctx)
      : context_(ctx), socket_(ctx) {}

  outcome::result<void> TcpConnection::close() {
    boost::system::error_code ec;
    socket_.close(ec);
    if (ec) {
      return handle_errcode(ec);
    }

    return outcome::success();
  }

  bool TcpConnection::isClosed() const {
    return !socket_.is_open();
  }

  outcome::result<multi::Multiaddress> TcpConnection::remoteMultiaddr() {
    return detail::makeAddress(socket_.remote_endpoint());
  }

  outcome::result<multi::Multiaddress> TcpConnection::localMultiaddr() {
    return detail::makeAddress(socket_.local_endpoint());
  }

  bool TcpConnection::isInitiator() const noexcept {
    return initiator_;
  }

  boost::system::error_code TcpConnection::handle_errcode(
      const boost::system::error_code &e) noexcept {
    // TODO(warchant): handle client disconnected; handle connection timeout
    ////      if (e.category() == boost::asio::error::get_misc_category()) {
    //        if (e.value() == boost::asio::error::eof) {
    //          using connection::emits::OnConnectionAborted;
    //          emit<OnConnectionAborted>(OnConnectionAborted{});
    //        }
    ////      }

    return e;
  }

  void TcpConnection::resolve(const TcpConnection::Tcp::endpoint &endpoint,
                              TcpConnection::ResolveCallbackFunc cb) {
    auto resolver = std::make_shared<Tcp::resolver>(context_);
    resolver->async_resolve(
        endpoint,
        [resolver, cb{std::move(cb)}](const ErrorCode &ec, auto &&iterator) {
          cb(ec, std::forward<decltype(iterator)>(iterator));
        });
  }

  void TcpConnection::connect(
      const TcpConnection::ResolverResultsType &iterator,
      TcpConnection::ConnectCallbackFunc cb) {
    boost::asio::async_connect(socket_, iterator,
                               [self{shared_from_this()}, cb{std::move(cb)}](
                                   auto &&ec, auto &&endpoint) {
                                 self->initiator_ = true;
                                 cb(std::forward<decltype(ec)>(ec),
                                    std::forward<decltype(endpoint)>(endpoint));
                               });
  }

  void TcpConnection::read(gsl::span<uint8_t> out,
                           TcpConnection::ReadCallbackFunc cb) {
    boost::asio::async_read(socket_, detail::makeBuffer(out), cb);
  }

  void TcpConnection::readSome(gsl::span<uint8_t> out,
                               TcpConnection::ReadCallbackFunc cb) {
    socket_.async_read_some(detail::makeBuffer(out), cb);
  }

  void TcpConnection::write(gsl::span<const uint8_t> in,
                            TcpConnection::WriteCallbackFunc cb) {
    boost::asio::async_write(socket_, detail::makeBuffer(in), cb);
  }

  void TcpConnection::writeSome(gsl::span<const uint8_t> in,
                                TcpConnection::WriteCallbackFunc cb) {
    socket_.async_write_some(detail::makeBuffer(in), cb);
  }

}  // namespace libp2p::transport
