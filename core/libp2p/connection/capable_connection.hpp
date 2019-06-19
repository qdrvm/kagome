/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CAPABLE_CONNECTION_HPP
#define KAGOME_CAPABLE_CONNECTION_HPP

#include <functional>

#include "libp2p/connection/secure_connection.hpp"
#include "libp2p/connection/stream.hpp"

namespace libp2p::connection {

  /**
   * Connection that provides basic libp2p requirements to the connection
   */
  struct CapableConnection : public SecureConnection {
    using StreamHandler = void(outcome::result<std::shared_ptr<Stream>>);
    using StreamHandlerFunc = std::function<StreamHandler>;

    ~CapableConnection() override = default;

    /**
     * Start to process incoming messages for this connection
     * @note non-blocking
     */
    virtual void start() = 0;

    /**
     * Stop processing incoming messages for this connection without closing the
     * connection itself
     * @note calling 'start' after 'close' is UB
     */
    virtual void stop() = 0;

    /**
     * @brief Opens new stream using this connection
     * @param cb - callback to be called, when a new stream is established or
     * error appears
     */
    virtual void newStream(StreamHandlerFunc cb) = 0;
  };

}  // namespace libp2p::connection

#endif  // KAGOME_CAPABLE_CONNECTION_HPP
