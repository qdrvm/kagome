/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TCP_SERVER_HPP
#define KAGOME_TCP_SERVER_HPP

#define BOOST_ASIO_NO_DEPRECATED

#include "libp2p/transport/asio/asio_server.hpp"

#include <boost/asio.hpp>
#include <outcome/outcome.hpp>
#include "libp2p/transport/asio/asio_server_factory.hpp"

namespace libp2p::transport {

  /**
   * @brief Async IPv4/IPv6 TCP Server.
   */
  class TcpServer final : public asio::Server {
   public:
    ~TcpServer() override = default;

    using Address = boost::asio::ip::address;
    using Tcp = boost::asio::ip::tcp;
    using Socket = typename Tcp::socket;
    using SocketResultCallback = void(outcome::result<Socket>);
    using HandlerFunc = std::function<SocketResultCallback>;

    static asio::ServerFactory::server_ptr_result create(
        boost::asio::io_context &context, const Tcp::endpoint &endpoint,
        HandlerFunc handler);

    void startAccept() override;

    bool isClosed() const override;

    outcome::result<void> close() override;

    multi::Multiaddress getMultiaddress() const override;

   private:
    TcpServer(boost::asio::io_context &context, HandlerFunc handler);

    boost::asio::io_context &context_;
    const HandlerFunc handler_;
    Tcp::acceptor acceptor_;
  };

}  // namespace libp2p::transport

#endif  //KAGOME_TCP_SERVER_HPP
