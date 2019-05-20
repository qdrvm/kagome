/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/transport/impl/transport_impl.hpp"

#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/transport/impl/transport_listener_impl.hpp"
#include "libp2p/transport/impl/transport_parser.hpp"
#include "libp2p/transport/tcp/tcp_connection.hpp"

namespace libp2p::transport {
  using multi::Multiaddress;
  using multi::Protocol;

  /**
   * Visitor for std::variant that returns the address data corresponding to the
   * protocol stored in the variant
   */
  class ParserVisitor {
    using Result = outcome::result<std::shared_ptr<Connection>>;

   public:
    explicit ParserVisitor(TransportImpl const &transport)
        : transport_{transport} {}

    /// IP/TCP address
    Result operator()(
        const std::pair<TransportParser::IpAddress, uint16_t> &ip_tcp) {
      return transport_.ipTcp(ip_tcp.first, ip_tcp.second);
    }

   private:
    TransportImpl const &transport_;
  };

  outcome::result<std::shared_ptr<Connection>> TransportImpl::dial(
      const multi::Multiaddress &address) const {
    OUTCOME_TRY(res, TransportParser::parse(address));
    ParserVisitor visitor{*this};
    return boost::apply_visitor(visitor, res.data_);
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
