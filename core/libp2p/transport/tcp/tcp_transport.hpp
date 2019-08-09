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
#include "libp2p/transport/transport_adaptor.hpp"
#include "libp2p/transport/upgrader.hpp"

namespace libp2p::transport {

  /**
   * @brief TCP Transport implementation
   */
  class TcpTransport : public TransportAdaptor,
                       public std::enable_shared_from_this<TcpTransport> {
   public:
    ~TcpTransport() override = default;

    TcpTransport(boost::asio::io_context &context,
                 std::shared_ptr<Upgrader> upgrader);

    void dial(const peer::PeerId &remoteId, multi::Multiaddress address,
              TransportAdaptor::HandlerFunc handler) override;

    std::shared_ptr<TransportListener> createListener(
        TransportListener::HandlerFunc handler) override;

    bool canDial(const multi::Multiaddress &ma) const override;

    peer::Protocol getProtocolId() const override;

   private:
    boost::asio::io_context &context_;
    std::shared_ptr<Upgrader> upgrader_;
  };  // namespace libp2p::transport

}  // namespace libp2p::transport

#endif  // KAGOME_TCP_TRANSPORT_HPP
