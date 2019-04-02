/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/transport/tcp/tcp_listener.hpp"

#include <boost/asio.hpp>
#include <boost/asio/post.hpp>
#include <boost/lexical_cast.hpp>
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/transport/tcp/tcp_connection.hpp"

using boost::asio::ip::address;
using boost::asio::ip::make_address;
using boost::asio::ip::tcp;
using libp2p::multi::Multiaddress;

namespace libp2p::transport {

  outcome::result<void> TcpListener::listen(const Multiaddress &address) {
    // TODO(warchant): there must be a better way to extract correct values
    OUTCOME_TRY(addr,
                address.getFirstValueForProtocol<boost::asio::ip::address>(
                    Multiaddress::Protocol::kIp4,
                    [](const std::string &val) { return make_address(val); }));

    OUTCOME_TRY(port,
                address.getFirstValueForProtocol<int>(
                    Multiaddress::Protocol::kTcp, [](const std::string &val) {
                      return boost::lexical_cast<int>(val);
                    }));

    try {
      tcp::endpoint endpoint(addr, port);
      acceptor_.open(endpoint.protocol());
      acceptor_.set_option(tcp::acceptor::reuse_address(true));
      acceptor_.bind(endpoint);
      acceptor_.listen();
    } catch (const boost::system::system_error &e) {
      return e.code();
    }

    // TODO(@warchant): maybe we need to store not the whole multiaddress, but
    // only ip4/tcp part?
    listening_on_.push_back(address);
    signal_start_listening_(address);

    // start async recv loop
    doAccept();

    return outcome::success();
  }

  outcome::result<void> TcpListener::close() {
    boost::system::error_code ec;
    acceptor_.close(ec);
    if (ec) {
      return ec;
    }

    listening_on_.clear();
    signal_close_();

    return outcome::success();
  }

  const std::vector<multi::Multiaddress> &TcpListener::getAddresses() const {
    return listening_on_;
  }

  TcpListener::TcpListener(boost::asio::io_context &io_context,
                           HandlerFunc handler)
      : acceptor_(io_context), handler_(std::move(handler)) {}

  void TcpListener::doAccept() {
    // async accept loop
    if (acceptor_.is_open()) {
      boost::asio::io_context &context = acceptor_.get_executor().context();
      auto session = std::make_shared<TcpConnection>(context);
      auto &socket = session->socket_;
      acceptor_.async_accept(
          socket,
          [&context, s = std::move(session),
           t = shared_from_this()](boost::system::error_code ec) {
            if (!ec) {
              boost::asio::post(context, [t, s]() {
                t->handler_(s);
                t->signal_new_connection_(s);
              });
            } else {
              t->signal_error_(ec);
            }

            t->doAccept();
          });
    }
  }

  bool TcpListener::isClosed() const noexcept {
    return !acceptor_.is_open();
  }

}  // namespace libp2p::transport
