/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/transport/tcp/tcp_connection.hpp"

namespace libp2p::transport {

  TcpConnection::TcpConnection(basic::yield_t &yield,
                               boost::asio::ip::tcp::socket &&socket)
      : yield_(yield), socket_(std::move(socket)) {}

  TcpConnection::TcpConnection(basic::yield_t &yield)
      : yield_(yield), socket_(yield.get_executor().context()) {}

  outcome::result<TcpConnection::ResolverResultsType> TcpConnection::resolve(
      const boost::asio::ip::tcp::endpoint &e) {
    Tcp::resolver resolver{yield_.get_executor().context()};
    auto [ec, iterator] = resolver.async_resolve(e, yield_);
    if (ec) {
      return handle_errcode(ec);
    }

    return iterator;
  }

  outcome::result<void> TcpConnection::connect(
      const boost::asio::ip::tcp::endpoint &endpoint) {
    OUTCOME_TRY(resolved, resolve(endpoint));
    initiator_ = true;
    auto [ec, e] = boost::asio::async_connect(socket_, resolved, yield_);
    if (ec) {
      return handle_errcode(ec);
    }
    return outcome::success();
  }

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
    return detail::makeEndpoint(socket_.remote_endpoint());
  }

  outcome::result<multi::Multiaddress> TcpConnection::localMultiaddr() {
    return detail::makeEndpoint(socket_.local_endpoint());
  }

  outcome::result<std::vector<uint8_t>> TcpConnection::read(size_t bytes) {
    std::vector<uint8_t> out(bytes, 0);
    OUTCOME_TRY(read(out));
    return out;
  }

  outcome::result<std::vector<uint8_t>> TcpConnection::readSome(size_t bytes) {
    std::vector<uint8_t> out(bytes, 0);
    OUTCOME_TRY(size, readSome(out));
    out.resize(size);
    return out;
  }

  outcome::result<size_t> TcpConnection::read(gsl::span<uint8_t> buf) {
    auto [ec, read] =
        boost::asio::async_read(socket_, detail::makeBuffer(buf), yield_);
    if (ec) {
      return handle_errcode(ec);
    }

    return read;
  }

  outcome::result<size_t> TcpConnection::readSome(gsl::span<uint8_t> buf) {
    auto [ec, read] = socket_.async_read_some(detail::makeBuffer(buf), yield_);
    if (ec) {
      return handle_errcode(ec);
    }

    return read;
  }

  outcome::result<size_t> TcpConnection::write(gsl::span<const uint8_t> in) {
    auto [ec, written] =
        boost::asio::async_write(socket_, detail::makeBuffer(in), yield_);
    if (ec) {
      return handle_errcode(ec);
    }

    return written;
  }

  outcome::result<size_t> TcpConnection::writeSome(
      gsl::span<const uint8_t> in) {
    auto [ec, written] =
        socket_.async_write_some(detail::makeBuffer(in), yield_);
    if (ec) {
      return handle_errcode(ec);
    }

    return written;
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
}  // namespace libp2p::transport
