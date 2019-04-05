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

  /**
   * @brief boost::asio implementation of TCP connection (socket).
   */
  class TcpConnection : public Connection,
                        public std::enable_shared_from_this<TcpConnection> {
   public:
    using Buffer = kagome::common::Buffer;
    using Tcp = boost::asio::ip::tcp;
    using ResolverResultsType = Tcp::resolver::results_type;
    using BufferResultCallback = void(outcome::result<kagome::common::Buffer>);

    explicit TcpConnection(boost::asio::io_context &context);
    explicit TcpConnection(boost::asio::io_context &context,
                           Tcp::socket socket);

    outcome::result<TcpConnection::ResolverResultsType> resolve(
        const Tcp::endpoint &e);

    outcome::result<void> connect(const Tcp::endpoint &endpoint);

    outcome::result<multi::Multiaddress> getRemoteMultiaddr() const override;

    outcome::result<Buffer> read(uint32_t to_read) override;

    outcome::result<Buffer> readSome(uint32_t to_read) override;

    void readAsync(
        std::function<BufferResultCallback> callback) noexcept override;

    outcome::result<void> writeSome(const Buffer &msg) override;

    outcome::result<void> write(const Buffer &msg) override;

    void writeAsync(const Buffer &msg,
                    std::function<ErrorCodeCallback> handler) noexcept override;

    outcome::result<void> close() override;

    bool isClosed() const override;

    ~TcpConnection() override = default;
    TcpConnection(const TcpConnection &copy) = delete;
    TcpConnection(TcpConnection &&move) = default;
    TcpConnection &operator=(const TcpConnection &other) = delete;
    TcpConnection &operator=(TcpConnection &&other) = default;

   private:
    Tcp::resolver resolver_;
    Tcp::socket socket_;
  };

}  // namespace libp2p::transport

#endif  // KAGOME_TCP_CONNECTION_HPP
