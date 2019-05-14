/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "transport_listener_impl.hpp"

#include <utility>

#include <boost/lexical_cast.hpp>
#include "libp2p/transport/tcp/tcp_server.hpp"
#include "libp2p/transport/tcp/tcp_connection.hpp"

namespace libp2p::transport {

  using boost::asio::ip::make_address;
  using multi::Multiaddress;
  using multi::Protocol;

  TransportListenerImpl::TransportListenerImpl(
      boost::asio::io_context &context, TransportListener::HandlerFunc handler)
      : context_(context), handler_(std::move(handler)) {}

  outcome::result<void> TransportListenerImpl::listen(
      const multi::Multiaddress &address) {
    // TODO(warchant): PRE-100 use parser here
    OUTCOME_TRY(addr,
                address.getFirstValueForProtocol<boost::asio::ip::address>(
                    Protocol::Code::IP4,
                    [](const std::string &val) { return make_address(val); }));

    OUTCOME_TRY(port,
                address.getFirstValueForProtocol<uint16_t>(
                    Protocol::Code::TCP, [](const std::string &val) {
                      return boost::lexical_cast<uint16_t>(val);
                    }));

    OUTCOME_TRY(server, ipTcp(addr, port));
    server->startAccept();
    signal_start_listening_(server->getMultiaddress());
    servers_.push_back(std::move(server));

    return outcome::success();
  }

  std::vector<multi::Multiaddress> TransportListenerImpl::getAddresses() const {
    std::vector<multi::Multiaddress> out;
    out.reserve(servers_.size());

    for (auto &&entry : servers_) {
      out.push_back(entry->getMultiaddress());
    }

    return out;
  }

  asio::ServerFactory::server_ptr_result TransportListenerImpl::ipTcp(
      const asio::ServerFactory::Address &ip, uint16_t port) const {
    return TcpServer::create(
        context_, {ip, port}, [this](outcome::result<TcpServer::Socket> r) {
          if (r) {
            auto c =
                std::make_shared<TcpConnection>(context_, std::move(r.value()));
            boost::asio::post(context_, [this, c]() { handler_(c); });
            signal_new_connection_(c);
          } else {
            signal_error_(r.error());
          }
        });
  }

  bool TransportListenerImpl::isClosed() const {
    return servers_.empty()
        || std::all_of(servers_.begin(), servers_.end(),
                       [](auto &&item) { return item->isClosed(); });
  }

  outcome::result<void> TransportListenerImpl::close() {
    for (auto &&item : servers_) {
      signal_close_(item->getMultiaddress());
      // ignore errors
      (void)item->close();
    }

    return outcome::success();
  }

  outcome::result<void> TransportListenerImpl::close(
      const multi::Multiaddress &ma) {
    for (auto &&item : servers_) {
      if (item->getMultiaddress() == ma) {
        signal_close_(ma);
        return item->close();
      }
    }

    return outcome::success();
  }

  boost::signals2::connection TransportListenerImpl::onClose(
      std::function<MultiaddrCallback> callback) {
    return signal_close_.connect(callback);
  }

  boost::signals2::connection TransportListenerImpl::onError(
      std::function<ErrorCallback> callback) {
    return signal_error_.connect(callback);
  }

  boost::signals2::connection TransportListenerImpl::onNewConnection(
      std::function<ConnectionCallback> callback) {
    return signal_new_connection_.connect(callback);
  }

  boost::signals2::connection TransportListenerImpl::onStartListening(
      std::function<MultiaddrCallback> callback) {
    return signal_start_listening_.connect(callback);
  }

}  // namespace libp2p::transport
