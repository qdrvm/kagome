/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TCP_CONNECTION_HPP
#define KAGOME_TCP_CONNECTION_HPP

#define BOOST_ASIO_NO_DEPRECATED

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <ufiber/ufiber.hpp>
#include "libp2p/basic/yield.hpp"
#include "libp2p/connection/raw_connection.hpp"
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/transport/tcp/tcp_util.hpp"

namespace libp2p::transport {

  /**
   * @brief boost::asio implementation of TCP connection (socket).
   */
  class TcpConnection : public connection::RawConnection,
                        public std::enable_shared_from_this<TcpConnection>,
                        private boost::noncopyable {
   public:
    using Tcp = boost::asio::ip::tcp;
    using ResolverResultsType = Tcp::resolver::results_type;

    TcpConnection(basic::yield_t &yield, Tcp::socket &&socket);

    explicit TcpConnection(basic::yield_t &yield);

    outcome::result<TcpConnection::ResolverResultsType> resolve(
        const Tcp::endpoint &e);

    outcome::result<void> connect(const Tcp::endpoint &endpoint);

    outcome::result<void> close() override;

    bool isClosed() const override;

    outcome::result<multi::Multiaddress> remoteMultiaddr() override;

    outcome::result<multi::Multiaddress> localMultiaddr() override;

    outcome::result<std::vector<uint8_t>> read(size_t bytes) override;

    outcome::result<std::vector<uint8_t>> readSome(size_t bytes) override;

    outcome::result<size_t> read(gsl::span<uint8_t> buf) override;

    outcome::result<size_t> readSome(gsl::span<uint8_t> buf) override;

    outcome::result<size_t> write(gsl::span<const uint8_t> in) override;

    outcome::result<size_t> writeSome(gsl::span<const uint8_t> in) override;

    bool isInitiator() const noexcept override;

   private:
    basic::yield_t &yield_;
    Tcp::socket socket_;
    bool initiator_ = false;

    boost::system::error_code handle_errcode(
        const boost::system::error_code &e) noexcept;
  };
}  // namespace libp2p::transport

#endif  // KAGOME_TCP_CONNECTION_HPP
