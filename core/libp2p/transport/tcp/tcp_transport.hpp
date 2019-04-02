/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TCP_TRANSPORT_HPP
#define KAGOME_TCP_TRANSPORT_HPP

#define BOOST_ASIO_NO_DEPRECATED

#include "libp2p/transport/transport.hpp"

#include <unordered_set>

#include <boost/asio.hpp>
#include "libp2p/transport/connection.hpp"

namespace libp2p::transport {

  /**
   * @brief Class that utilizes boost::asio to create tcp servers (listeners)
   * and connect to other servers (dial).
   */
  class TcpTransport final: public Transport {
   public:
    explicit TcpTransport(boost::asio::io_context &context);

    outcome::result<std::shared_ptr<Connection>> dial(
        const multi::Multiaddress &address) final;

    std::shared_ptr<TransportListener> createListener(
        TransportListener::HandlerFunc handler) final;

    ~TcpTransport() final = default;

    TcpTransport(const TcpTransport &copy) = default;
    TcpTransport(TcpTransport &&move) = default;
    TcpTransport &operator=(const TcpTransport &other) = delete;
    TcpTransport &operator=(TcpTransport &&other) = delete;

   private:
    boost::asio::io_context &context_;

    std::unordered_set<std::shared_ptr<TransportListener>> listeners_;
  };

}  // namespace libp2p::transport

#endif  // KAGOME_TCP_TRANSPORT_HPP
