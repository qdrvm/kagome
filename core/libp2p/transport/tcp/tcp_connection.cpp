/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/transport/tcp/tcp_connection.hpp"

#include <sstream>

namespace libp2p::transport {

  TcpConnection::TcpConnection(boost::asio::io_context &context)
      : socket_(context), resolver_(context) {}

  TcpConnection::TcpConnection(boost::asio::io_context &context, boost::asio::ip::tcp::socket socket)
      : socket_(std::move(socket)),
        resolver_(context) {}

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

  outcome::result<kagome::common::Buffer> TcpConnection::readSome(
      uint32_t to_read) {
    boost::system::error_code ec;
    kagome::common::Buffer buf(to_read, 0);

    size_t len =
        socket_.read_some(boost::asio::buffer(buf.toVector(), to_read), ec);
    if (len > 0 && !ec) {
      return buf.resize(len);  // we could receive less than to_read
    }

    return ec;
  }

  outcome::result<kagome::common::Buffer> TcpConnection::read(
      uint32_t to_read) {
    boost::system::error_code ec;
    kagome::common::Buffer buf(to_read, 0);

    std::size_t len = boost::asio::read(
        socket_, boost::asio::buffer(buf.toVector(), to_read), ec);
    if (len > 0 && !ec) {
      return buf;  // we receive always `to_read` bytes
    }

    return ec;
  }

  outcome::result<void> TcpConnection::writeSome(
      const kagome::common::Buffer &msg) {
    boost::system::error_code ec;
    socket_.write_some(boost::asio::buffer(msg.toVector()), ec);
    if (ec) {
      return ec;
    }

    return outcome::success();
  }

  outcome::result<void> TcpConnection::write(
      const kagome::common::Buffer &msg) {
    boost::system::error_code ec;
    boost::asio::write(socket_, boost::asio::buffer(msg.toVector()), ec);
    if (ec) {
      return ec;
    }

    return outcome::success();
  }

  void TcpConnection::writeAsync(
      const kagome::common::Buffer &msg,
      std::function<ErrorCodeCallback> handler) noexcept {
    socket_.async_write_some(boost::asio::buffer(msg.toVector()), handler);
  }

  void TcpConnection::readAsync(
      std::function<BufferResultCallback> callback) noexcept {
    socket_.async_wait(
        Tcp::socket::wait_read,
        [cb = std::move(callback),
         s = shared_from_this()](boost::system::error_code ec) {
          if (!ec) {
            // lazily allocate buffer of required size
            kagome::common::Buffer lazy(s->socket_.available(), 0);

            // TODO(@warchant): review this. it may not be the best design.
            size_t size =
                s->socket_.read_some(boost::asio::buffer(lazy.toVector()), ec);
            lazy.resize(size);
            cb(lazy);
          } else {
            cb(ec);
          }
        });
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

}  // namespace libp2p::transport
