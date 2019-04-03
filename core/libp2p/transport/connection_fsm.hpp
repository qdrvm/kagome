/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONNECTION_FSM_HPP
#define KAGOME_CONNECTION_FSM_HPP

#include <vector>

#include "libp2p/transport/connection.hpp"
#include "libp2p/error/error.hpp"

namespace libp2p::transport {
  /**
   * Finite-state machine of the connection; represents connection's status
   */
  class ConnectionFSM {
    /**
     * Connection is successfully established
     * @param callback to be called, when event happens
     */
    virtual void onConnection(
        std::function<void(const connection::Connection &conn)> callback)
        const = 0;

    /**
     * Connection is closed
     * @param callback to be called, when event happens
     */
    virtual void onClose(std::function<void()> callback) const = 0;

    /**
     * Fatal error occurs with the connection
     * @param callback to be called, when event happens
     */
    virtual void onError(
        std::function<void(const error::Error &error)> callback) const = 0;

    /**
     * Connection fails to be upgraded with a muxer
     * @param callback to be called, when event happens
     */
    virtual void onUpgradeFailed(
        std::function<void(const error::Error &error)> callback) const = 0;

    /**
     * A dial attempt fails for a given transport
     * @param callback to be called, when event happens
     */
    virtual void onConnectionAttemptFailed(
        std::function<void(std::vector<error::Error> errors)> callback)
        const = 0;
  };
}  // namespace libp2p::connection

#endif  // KAGOME_CONNECTION_FSM_HPP
