/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_METRICS_EXPOSER_HPP
#define KAGOME_CORE_METRICS_EXPOSER_HPP

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <memory>
#include "handler.hpp"

namespace kagome::metrics {

  class Exposer {
   protected:
    using Acceptor = boost::asio::ip::tcp::acceptor;
    using Endpoint = boost::asio::ip::tcp::endpoint;

   public:
    using Context = boost::asio::io_context;

    struct Configuration {
      Endpoint endpoint{};

      Configuration() {
        endpoint.address(boost::asio::ip::address_v4::any());
        endpoint.port(0);
      }
    };

    void setHandler(const std::shared_ptr<Handler> &handler) {
      handler_ = handler;
    }

    virtual ~Exposer() = default;

    virtual bool prepare() = 0;
    virtual bool start() = 0;
    virtual void stop() = 0;

   protected:
    std::shared_ptr<Handler> handler_;
  };

}  // namespace kagome::metrics

#endif  // KAGOME_CORE_METRICS_EXPOSER_HPP
