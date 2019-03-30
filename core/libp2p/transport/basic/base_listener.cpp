/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "base_listener.hpp"

namespace libp2p::transport {

  boost::signals2::connection BaseListener::onClose(
      std::function<NoArgsCallback> callback) {
    return signal_close_.connect(callback);
  }

  boost::signals2::connection BaseListener::onError(
      std::function<ErrorCallback> callback) {
    return signal_error_.connect(callback);
  }

  boost::signals2::connection BaseListener::onNewConnection(
      std::function<ConnectionCallback> callback) {
    return signal_new_connection_.connect(callback);
  }

  boost::signals2::connection BaseListener::onStartListening(
      std::function<MultiaddrCallback> callback) {
    return signal_start_listening_.connect(callback);
  }

}  // namespace libp2p::transport
