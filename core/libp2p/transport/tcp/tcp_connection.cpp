/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/transport/tcp/tcp_connection.hpp"

#include <sstream>

namespace libp2p::transport {

  TcpConnection::TcpConnection(boost::asio::io_context &context)
      : socket_(context), resolver_(context) {}

  TcpConnection::TcpConnection(boost::asio::io_context &context,
                               boost::asio::ip::tcp::socket socket)
      : socket_(std::move(socket)), resolver_(context) {}

  outcome::result<multi::Multiaddress> TcpConnection::getRemoteMultiaddr()
      const {
    try {
      auto re = socket_.remote_endpoint();
      auto address = re.address();
      auto port = re.port();

      std::ostringstream s;
      if (address.is_v4()) {
        s << "/ip4/" << address.to_v4().to_string();
      } else {
        s << "/ip6/" << address.to_v6().to_string();
      }

      s << "/tcp/" << port;

      return multi::Multiaddress::create(s.str());
    } catch (const boost::system::system_error &e) {
      return e.code();
    }
  }

  outcome::result<void> TcpConnection::connect(const Tcp::endpoint &endpoint) {
    OUTCOME_TRY(resolved, resolve(endpoint));

    boost::system::error_code ec;
    boost::asio::connect(socket_, resolved, ec);
    if (ec) {
      return ec;
    }

    return outcome::success();
  }

  outcome::result<TcpConnection::ResolverResultsType> TcpConnection::resolve(
      const Tcp::endpoint &endpoint) {
    boost::system::error_code ec;
    auto iterator = resolver_.resolve(endpoint, ec);
    if (ec) {
      return ec;
    }

    return iterator;
  }

  outcome::result<void> TcpConnection::close() {
    boost::system::error_code ec;
    socket_.close(ec);
    if (ec) {
      return ec;
    }

    return outcome::success();
  }

  bool TcpConnection::isClosed() const {
    return !socket_.is_open();
  }

  void TcpConnection::asyncRead(
      boost::asio::mutable_buffer &mut, uint32_t to_read,
      std::function<Readable::CompletionHandler> cb) {
    boost::asio::async_read(
        socket_, mut, boost::asio::transfer_exactly(to_read),
        [cb = std::move(cb)](const boost::system::error_code &ec, size_t size) {
          cb(ec, size);
        });
  }

  void TcpConnection::asyncWrite(
      const boost::asio::const_buffer &buf,
      std::function<Writable::CompletionHandler> cb) {
    boost::asio::async_write(
        socket_, buf,
        [cb = std::move(cb)](const boost::system::error_code &ec, size_t size) {
          cb(ec, size);
        });
  }

  void TcpConnection::asyncRead(boost::asio::mutable_buffer &&mut,
                                uint32_t to_read,
                                std::function<Readable::CompletionHandler> cb) {
    boost::asio::async_read(
        socket_, mut, boost::asio::transfer_exactly(to_read),
        [cb = std::move(cb)](const boost::system::error_code &ec, size_t size) {
          cb(ec, size);
        });
  }

  void TcpConnection::asyncRead(boost::asio::streambuf &streambuf,
                                uint32_t to_read,
                                std::function<Readable::CompletionHandler> cb) {
    boost::asio::async_read(
        socket_, streambuf, boost::asio::transfer_exactly(to_read),
        [cb = std::move(cb)](const boost::system::error_code &ec, size_t size) {
          cb(ec, size);
        });
  }

  void TcpConnection::asyncWrite(
      boost::asio::streambuf &buf,
      std::function<Writable::CompletionHandler> cb) {
    boost::asio::async_write(
        socket_, buf,
        [cb = std::move(cb)](const boost::system::error_code &ec, size_t size) {
          cb(ec, size);
        });
  }

}  // namespace libp2p::transport
