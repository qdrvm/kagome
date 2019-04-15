/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSPORT_IMPL_HPP
#define KAGOME_TRANSPORT_IMPL_HPP

#define BOOST_ASIO_NO_DEPRECATED

#include "libp2p/transport/transport.hpp"

#include <boost/asio/io_context.hpp>
#include "libp2p/transport/asio/asio_client_factory.hpp"

namespace libp2p::transport {
  class TransportImpl : public Transport, public asio::ClientFactory {
   public:
    ~TransportImpl() override = default;

    explicit TransportImpl(boost::asio::io_context &context);

    outcome::result<std::shared_ptr<Connection>> dial(
        const multi::Multiaddress &address) const override;

    std::shared_ptr<TransportListener> createListener(
        TransportListener::HandlerFunc handler) const override;

    client_ptr_result ipTcp(const Address &ip, uint16_t port) const override;

   private:
    boost::asio::io_context &context_;
  };

}  // namespace libp2p::transport

#endif  // KAGOME_TRANSPORT_IMPL_HPP
