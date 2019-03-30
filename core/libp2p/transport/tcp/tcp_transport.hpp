/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TCP_TRANSPORT_HPP
#define KAGOME_TCP_TRANSPORT_HPP

#define BOOST_ASIO_NO_DEPRECATED

#include <boost/asio.hpp>
#include <unordered_set>
#include "libp2p/transport/connection.hpp"
#include "libp2p/transport/transport.hpp"

namespace libp2p::transport {

  /**
   * @brief Class that utilizes boost::asio to create tcp servers (listeners)
   * and connect to other servers (dial).
   */
  class TcpTransport : public Transport {
   public:
    explicit TcpTransport(boost::asio::io_context &context);

    outcome::result<std::shared_ptr<Connection>> dial(
        const multi::Multiaddress &address) final;

    std::shared_ptr<TransportListener> createListener(
        TransportListener::HandlerFunc handler) final;

    bool isClosed() const final;

    std::error_code close() final;

    ~TcpTransport() final = default;

   private:
    boost::asio::io_context &context_;

    std::unordered_set<std::shared_ptr<TransportListener>> listeners_;
  };

}  // namespace libp2p::transport

#endif  // KAGOME_TCP_TRANSPORT_HPP
