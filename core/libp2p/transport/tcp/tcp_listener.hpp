/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TCP_LISTENER_HPP
#define KAGOME_TCP_LISTENER_HPP

#define BOOST_ASIO_NO_DEPRECATED

#include "libp2p/transport/basic/base_listener.hpp"

#include <string>

#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include <outcome/outcome.hpp>

namespace libp2p::transport {

  /**
   * @brief Single threaded asynchronous TCP/IPv4 server implementation.
   */
  class TcpListener : public BaseListener {
   public:
    TcpListener(boost::asio::io_context &io_context, HandlerFunc handler);

    outcome::result<void> listen(const multi::Multiaddress &address) override;

    const std::vector<multi::Multiaddress> &getAddresses() const override;

    bool isClosed() const noexcept override;

    outcome::result<void> close() override;

    ~TcpListener() override = default;

    TcpListener(const TcpListener &copy) = delete;
    TcpListener(TcpListener &&move) = default;
    TcpListener &operator=(const TcpListener &other) = delete;
    TcpListener &operator=(TcpListener &&other) = delete;

   private:
    boost::asio::ip::tcp::acceptor acceptor_;
    const HandlerFunc handler_;

    std::vector<multi::Multiaddress> listening_on_;

    void doAccept();
  };

}  // namespace libp2p::transport

#endif  // KAGOME_TCP_LISTENER_HPP
