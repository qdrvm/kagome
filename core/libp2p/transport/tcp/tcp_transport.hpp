/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TCP_TRANSPORT_HPP
#define KAGOME_TCP_TRANSPORT_HPP

#define BOOST_ASIO_NO_DEPRECATED

#include <boost/asio.hpp>
#include <ufiber/ufiber.hpp>
#include "libp2p/transport/tcp/tcp_listener.hpp"
#include "libp2p/transport/tcp/tcp_util.hpp"
#include "libp2p/transport/transport.hpp"

namespace libp2p::transport {

  /**
   * @brief TCP Transport implementation
   * @tparam Executor executor type - io context executor, strand or custom
   */
  template <typename Executor,
            typename = typename std::enable_if<
                boost::asio::is_executor<Executor>::value>::type>
  class TcpTransport
      : public Transport,
        public std::enable_shared_from_this<TcpTransport<Executor>> {
   public:
    using executor_t = Executor;

    explicit TcpTransport(Executor &executor) : executor_(executor) {}

    void dial(const multi::Multiaddress &address,
              Transport::HandlerFunc onSuccess,
              Transport::ErrorFunc onError) const override {
      ufiber::spawn(
          executor_,
          [address, v{std::move(onSuccess)},
           e{std::move(onError)}](auto &&yield) mutable {
            // same as canDial, but we do not need to pass transport instance
            // inside lambda
            if (!detail::supportsIpTcp(address)) {
              std::error_code ec =
                  make_error_code(std::errc::address_family_not_supported);
              return e(ec);
            }

            auto conn = std::make_shared<TcpConnection<Executor>>(yield);
            auto rendpoint = detail::makeEndpoint(address);
            if (!rendpoint) {
              return e(rendpoint.error());
            }

            auto r1 = conn->connect(rendpoint.value());
            if (!r1) {
              return e(r1.error());
            }

            // execute handler
            auto r2 = v(std::move(conn));
            if (!r2) {
              // report error, if user specified callback returned error.
              return e(r2.error());
            }
          });
    };

    std::shared_ptr<TransportListener> createListener(
        TransportListener::HandlerFunc onValue,
        TransportListener::ErrorFunc onError) const override {
      return std::make_shared<TcpListener<Executor>>(
          executor_, std::move(onValue), std::move(onError));
    };

    bool canDial(const multi::Multiaddress &ma) const override {
      return detail::supportsIpTcp(ma);
    };

   private:
    Executor &executor_;
  };  // namespace libp2p::transport

}  // namespace libp2p::transport

#endif  // KAGOME_TCP_TRANSPORT_HPP
