/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TCP_CONNECTION_HPP
#define KAGOME_TCP_CONNECTION_HPP

#define BOOST_ASIO_NO_DEPRECATED

#include "libp2p/transport/connection.hpp"

#include <array>

#include <boost/asio.hpp>

namespace libp2p::transport {

  class TcpListener;

  // TODO(warchant): consider using of boost::asio::streambuf for better
  // performance/usability instead of kagome::common::Buffer

  // TODO(warchant): consider using pre-allocated buffer for better performance.
  // This buffer should have size = upper size of single message.

  /**
   * @brief boost::asio implementation of TCP connection (socket).
   */
  class TcpConnection : public Connection,
                        public std::enable_shared_from_this<TcpConnection> {
    using Socket = boost::asio::ip::tcp::socket;
    using Buffer = kagome::common::Buffer;

   public:
    explicit TcpConnection(boost::asio::io_context &context);
    explicit TcpConnection(Socket socket);

    outcome::result<multi::Multiaddress> getRemoteMultiaddr() const override;

    outcome::result<Buffer> read(uint32_t to_read) override;

    outcome::result<Buffer> readSome(uint32_t to_read) override;

    void readAsync(
        std::function<BufferResultCallback> callback) noexcept override;

    outcome::result<void> writeSome(const Buffer &msg) override;

    outcome::result<void> write(const Buffer &msg) override;

    void writeAsync(const Buffer &msg,
                    std::function<ErrorCodeCallback> handler) noexcept override;

    /**
     * @throws boost::system::system_error
     */
    void close() override;

    bool isClosed() const override;

    ~TcpConnection() override = default;
    TcpConnection(const TcpConnection &copy) = delete;
    TcpConnection(TcpConnection &&move) = default;
    TcpConnection &operator=(const TcpConnection &other) = delete;
    TcpConnection &operator=(TcpConnection &&other) = default;

   private:
    friend class TcpListener;

    Socket socket_;
  };

}  // namespace libp2p::transport

#endif  // KAGOME_TCP_CONNECTION_HPP
