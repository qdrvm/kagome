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

    explicit TcpConnection(boost::asio::io_context &context);
    explicit TcpConnection(boost::asio::io_context &context,
                           Tcp::socket socket);

    outcome::result<TcpConnection::ResolverResultsType> resolve(
        const Tcp::endpoint &e);

    outcome::result<void> connect(const Tcp::endpoint &endpoint);

    outcome::result<multi::Multiaddress> getRemoteMultiaddr() const override;

    outcome::result<void> close() override;

    bool isClosed() const override;

    void asyncRead(boost::asio::mutable_buffer &mut, uint32_t to_read,
                   std::function<Readable::CompletionHandler> cb) override;

    void asyncRead(boost::asio::mutable_buffer &&mut, uint32_t to_read,
                   std::function<Readable::CompletionHandler> cb) override;

    void asyncRead(boost::asio::streambuf &streambuf, uint32_t to_read,
                   std::function<Readable::CompletionHandler> cb) override;

    void asyncWrite(const boost::asio::const_buffer &buf,
                    std::function<Writable::CompletionHandler> cb) override;

    void asyncWrite(boost::asio::streambuf &buf,
                    std::function<Writable::CompletionHandler> cb) override;

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
