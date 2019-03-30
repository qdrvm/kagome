/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BASE_LISTENER_HPP
#define KAGOME_BASE_LISTENER_HPP

#define BOOST_ASIO_NO_DEPRECATED

#include "libp2p/transport/transport_listener.hpp"

#include <boost/signals2.hpp>

namespace libp2p::transport {

  /**
   * @brief Base class for all Listeners, so they don't need to duplicate
   * signals
   */
  class BaseListener : public TransportListener {
   public:
    using NoArgsSignal = boost::signals2::signal<NoArgsCallback>;
    using MultiaddrSignal = boost::signals2::signal<MultiaddrCallback>;
    using ErrorSignal = boost::signals2::signal<ErrorCallback>;
    using ConnectionSignal = boost::signals2::signal<ConnectionCallback>;

    boost::signals2::connection onStartListening(
        std::function<MultiaddrCallback> callback) override;

    boost::signals2::connection onNewConnection(
        std::function<ConnectionCallback> callback) override;

    boost::signals2::connection onError(
        std::function<ErrorCallback> callback) override;

    boost::signals2::connection onClose(
        std::function<NoArgsCallback> callback) override;

    ~BaseListener() override = default;

   protected:
    MultiaddrSignal signal_start_listening_{};
    ConnectionSignal signal_new_connection_{};
    ErrorSignal signal_error_{};
    NoArgsSignal signal_close_{};
  };
}  // namespace libp2p::transport

#endif  // KAGOME_BASE_LISTENER_HPP
