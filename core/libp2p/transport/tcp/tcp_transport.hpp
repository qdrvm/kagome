/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TCP_TRANSPORT_HPP
#define KAGOME_TCP_TRANSPORT_HPP

#define BOOST_ASIO_NO_DEPRECATED

#include <boost/asio.hpp>
#include "libp2p/transport/tcp/tcp_listener.hpp"
#include "libp2p/transport/tcp/tcp_util.hpp"
#include "libp2p/transport/transport.hpp"
#include "libp2p/transport/upgrader.hpp"

namespace libp2p::transport {

  /**
   * @brief TCP Transport implementation
   */
  class TcpTransport : public Transport,
                       public std::enable_shared_from_this<TcpTransport> {
   public:
    TcpTransport(boost::asio::io_context &context,
                 std::shared_ptr<Upgrader> upgrader);

    void dial(multi::Multiaddress address,
              Transport::HandlerFunc handler) override;

    std::shared_ptr<TransportListener> createListener(
        TransportListener::HandlerFunc handler) override;

    bool canDial(const multi::Multiaddress &ma) const override;

   private:
    boost::asio::io_context &context_;
    std::shared_ptr<Upgrader> upgrader_;

    void onResolved(std::shared_ptr<TcpConnection> c,
                    const TcpConnection::ResolverResultsType &r,
                    Transport::HandlerFunc handler);

    void onConnected(std::shared_ptr<TcpConnection> c,
                     Transport::HandlerFunc handler);

    void onConnSecured(std::shared_ptr<connection::SecureConnection> c,
                       Transport::HandlerFunc handler);
  };  // namespace libp2p::transport

}  // namespace libp2p::transport

#endif  // KAGOME_TCP_TRANSPORT_HPP
