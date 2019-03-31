/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/transport/tcp/tcp_connection.hpp"

#include <sstream>

namespace libp2p::transport {

  TcpConnection::TcpConnection(boost::asio::io_context &context)
      : socket_(context) {}

  TcpConnection::TcpConnection(boost::asio::ip::tcp::socket socket)
      : socket_(std::move(socket)) {}

  outcome::result<std::vector<multi::Multiaddress>>
  TcpConnection::getObservedAddresses() const {
    try {
      auto &&remote = socket_.remote_endpoint();
      auto &&addr = remote.address().to_string();
      auto &&port = remote.port();
      std::ostringstream s{};
      s << "/ip4/" << addr << "/tcp/" << port;
      OUTCOME_TRY(ma, multi::Multiaddress::create(s.str()));
      return std::vector<multi::Multiaddress>{ma};
    } catch (const boost::system::system_error &e) {
      return e.code();
    }
  }

  std::optional<common::PeerInfo> TcpConnection::getPeerInfo() const {
    return info_;
  }

  void TcpConnection::setPeerInfo(const common::PeerInfo &info) {
    info_ = info;
  }

  outcome::result<kagome::common::Buffer> TcpConnection::readSome(
      uint32_t to_read) {
    boost::system::error_code ec;
    kagome::common::Buffer buf(to_read, 0);

    std::size_t len =
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

  std::error_code TcpConnection::writeSome(const kagome::common::Buffer &msg) {
    boost::system::error_code ec;
    socket_.write_some(boost::asio::buffer(msg.toVector()), ec);
    return ec;
  }

  std::error_code TcpConnection::write(const kagome::common::Buffer &msg) {
    boost::system::error_code ec;
    boost::asio::write(socket_, boost::asio::buffer(msg.toVector()), ec);
    return ec;
  }

  void TcpConnection::writeAsync(
      const kagome::common::Buffer &msg,
      std::function<ErrorCodeCallback> handler) noexcept {
    socket_.async_write_some(boost::asio::buffer(msg.toVector()), handler);
  }

  void TcpConnection::readAsync(
      std::function<BufferResultCallback> callback) noexcept {
    socket_.async_wait(
        boost::asio::ip::tcp::socket::wait_read,
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

  std::error_code TcpConnection::close() {
    boost::system::error_code ec;
    socket_.close(ec);
    return ec;
  }

  bool TcpConnection::isClosed() const {
    return !socket_.is_open();
  }

}  // namespace libp2p::transport
