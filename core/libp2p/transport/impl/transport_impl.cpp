/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/transport/impl/transport_impl.hpp"

#include <boost/asio/ip/address.hpp>
#include <boost/lexical_cast.hpp>
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/transport/impl/transport_listener_impl.hpp"
#include "libp2p/transport/tcp/tcp_connection.hpp"

namespace libp2p::transport {
  using boost::asio::ip::make_address;
  using multi::Multiaddress;

  outcome::result<std::shared_ptr<Connection>> TransportImpl::dial(
      const multi::Multiaddress &address) const {
    // TODO(warchant): PRE-100 use parser here
    OUTCOME_TRY(addr,
                address.getFirstValueForProtocol<boost::asio::ip::address>(
                    Multiaddress::Protocol::IP_4,
                    [](const std::string &val) { return make_address(val); }));

    OUTCOME_TRY(port,
                address.getFirstValueForProtocol<uint16_t>(
                    Multiaddress::Protocol::TCP, [](const std::string &val) {
                      return boost::lexical_cast<uint16_t>(val);
                    }));

    return ipTcp(addr, port);
  }

  std::shared_ptr<TransportListener> TransportImpl::createListener(
      TransportListener::HandlerFunc handler) const {
    return std::make_shared<TransportListenerImpl>(context_,
                                                   std::move(handler));
  }

  TransportImpl::TransportImpl(boost::asio::io_context &context)
      : context_(context) {}

  asio::ClientFactory::client_ptr_result TransportImpl::ipTcp(
      const asio::ClientFactory::Address &ip, uint16_t port) const {
    auto client = std::make_shared<TcpConnection>(context_);
    OUTCOME_TRY(client->connect({ip, port}));
    return client;
  }
}  // namespace libp2p::transport
