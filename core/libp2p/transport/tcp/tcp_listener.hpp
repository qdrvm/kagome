/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TCP_LISTENER_HPP
#define KAGOME_TCP_LISTENER_HPP

#include <boost/asio.hpp>
#include <ufiber/ufiber.hpp>
#include "libp2p/transport/tcp/tcp_connection.hpp"
#include "libp2p/transport/tcp/tcp_util.hpp"
#include "libp2p/transport/transport_listener.hpp"

namespace libp2p::transport {

  /**
   * @brief TCP Server (Listener) implementation.
   * @tparam Executor asio executor type - can be a io_context, strand, or custom.
   */
  template <typename Executor,
            typename = typename std::enable_if<
                boost::asio::is_executor<Executor>::value>::type>
  class TcpListener
      : public TransportListener,
        public std::enable_shared_from_this<TcpListener<Executor>> {
   public:
    TcpListener(Executor &executor, TransportListener::HandlerFunc onValue,
                TransportListener::ErrorFunc onError)
        : acceptor_(executor.context()),
          v_(std::move(onValue)),
          e_(std::move(onError)) {}

    outcome::result<void> listen(const multi::Multiaddress &address) override {
      if (!canListen(address)) {
        return std::errc::address_family_not_supported;
      }

      if (acceptor_.is_open()) {
        return std::errc::already_connected;
      }

      // TODO(@warchant): replace with parser PRE-129
      using namespace boost::asio;  // NOLINT
      try {
        // throws
        auto endpoint = detail::make_endpoint(address);

        // setup acceptor, throws
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(ip::tcp::acceptor::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen();

        // start listening
        do_accept();

        return outcome::success();
      } catch (const boost::system::system_error &e) {
        return e.code();
      } catch (const boost::bad_lexical_cast &e) {
        return multi::Multiaddress::Error::INVALID_PROTOCOL_VALUE;
      }
    };

    bool canListen(const multi::Multiaddress &ma) const override {
      return detail::supports_ip_tcp(ma);
    };

    outcome::result<multi::Multiaddress> getListenMultiaddr() const override {
      try {
        return detail::make_endpoint(acceptor_.local_endpoint());
      } catch (const boost::system::system_error &e) {
        return e.code();
      }
    };

    bool isClosed() const override {
      return !acceptor_.is_open();
    };

    outcome::result<void> close() override {
      boost::system::error_code ec;
      acceptor_.close(ec);
      return ec;
    };

   private:
    boost::asio::ip::tcp::acceptor acceptor_;
    TransportListener::HandlerFunc v_;
    TransportListener::ErrorFunc e_;

    void do_accept() {
      using namespace boost::asio;    // NOLINT
      using namespace boost::system;  // NOLINT

      if (!acceptor_.is_open()) {
        return;
      }

      acceptor_.async_accept(
          [self{this->shared_from_this()}](const boost::system::error_code &ec,
                                           ip::tcp::socket sock) {
            if (ec) {
              return self->e_(ec);
            }

            ufiber::spawn(self->acceptor_.get_executor(),
                          [sock{std::move(sock)},
                           self{std::move(self)}](auto &&yield) mutable {
                            using E = decltype(yield.get_executor());
                            auto conn = std::make_shared<TcpConnection<E>>(
                                yield, std::move(sock));
                            // execute user-provided callback
                            auto r = self->v_(std::move(conn));
                            if (!r) {
                              // signal error in case of failure
                              self->e_(r.error());
                            }
                          });

            self->do_accept();
          });
    }
  };

}  // namespace libp2p::transport

#endif  // KAGOME_TCP_LISTENER_HPP
