/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_TRANSPORT_IMPL_BASIC_TCP_TRANSPORT_HPP
#define KAGOME_CORE_API_TRANSPORT_IMPL_BASIC_TCP_TRANSPORT_HPP

#include <boost/asio/io_context.hpp>
#include "api/transport/basic_transport.hpp"
#include "api/transport/network_address.hpp"

namespace kagome::api {
  class TcpTransportImpl : public BasicTransport {
    using network_address_t = NetworkAddress;
    using network_port_t = uint16_t;

   public:
    TcpTransportImpl(boost::asio::io_context &context,
                     network_address_t address, network_port_t port)
        : context_(context), address_{std::move(address)}, port_{port} {}

   protected:
    boost::asio::io_context &context_;
    network_address_t address_;
    network_port_t port_;
  };
}  // namespace kagome::api

#endif  // KAGOME_CORE_API_TRANSPORT_IMPL_BASIC_TCP_TRANSPORT_HPP
