/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TELEMETRY_CONNECTION_HPP
#define KAGOME_TELEMETRY_CONNECTION_HPP

#include <functional>
#include <memory>
#include <string>

#include "telemetry/endpoint.hpp"

namespace kagome::telemetry {

  /**
   * Represents a connection to the single telemetry server.
   *
   * The target URI and OnConnectedCallback are to be passed to the
   * implementation class' constructor.
   */
  class TelemetryConnection {
   public:
    /**
     * The callback to be called each time the connection (re-)establishes.
     *
     * Can be called multiple times even after a single call of connect()
     * due to losing connection and reconnecting to the backend server.
     *
     * The callback is used to let the TelemetryService send the greeting
     * message.
     */
    using OnConnectedCallback =
        std::function<void(std::shared_ptr<TelemetryConnection>)>;

    virtual ~TelemetryConnection() = default;

    /**
     * Initiates attempts to connect.
     *
     * Designed to be called only once by the TelemetryService.
     */
    virtual void connect() = 0;

    /// Returns the associated telemetry endpoint
    virtual const TelemetryEndpoint &endpoint() const = 0;

    /**
     * Write the data to the websocket if connected
     * @param data - the data line to send
     *
     * The data might be disposed in an outer scope
     * as soon as the method returns.
     */
    virtual void send(const std::string &data) = 0;

    /**
     * Write the message pointed by a message handle.
     * @param message_handle - message to serve
     *
     * TelemetryConnection and TelemetryService are tightly related and shares a
     * common message pool to avoid redundant memory consumption. That is why
     * TelemetryService schedules messages to the pool and passes only handles
     * to connections. It is connection's duty to release a message from the
     * pool when send/write operation completes.
     */
    virtual void send(std::size_t message_handle) = 0;

    /// Get the current status of the connection
    virtual bool isConnected() const = 0;

    /// Request the connection to shutdown
    virtual void shutdown() = 0;
  };

}  // namespace kagome::telemetry

#endif  // KAGOME_TELEMETRY_CONNECTION_HPP
