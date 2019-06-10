/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CAPABLE_CONNECTION_HPP
#define KAGOME_CAPABLE_CONNECTION_HPP

#include "libp2p/connection/secure_connection.hpp"
#include "libp2p/connection/stream.hpp"

namespace libp2p::connection {

  // connection that provides basic libp2p requirements to the connection
  struct CapableConnection : public SecureConnection {
    using StreamResultHandler = void(outcome::result<std::shared_ptr<Stream>>);

    ~CapableConnection() override = default;

    /**
     * Start to process incoming messages for this connection
     * @return nothing or error
     * @note this method is successfully executed (returns void) iff underlying
     * connection was closed on our side via close() method; for example, if the
     * connection wad closed from the other side, EOF boost error is going to be
     * returned
     */
    virtual outcome::result<void> start() = 0;

    /**
     * @brief Opens new stream using this connection
     * @param stream_handler to be called, when a stream is successfully opened
     */
    virtual void newStream(
        std::function<StreamResultHandler> stream_handler) = 0;
  };

}  // namespace libp2p::connection

#endif  // KAGOME_CAPABLE_CONNECTION_HPP
