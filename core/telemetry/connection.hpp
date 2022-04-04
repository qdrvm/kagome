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
   * Represents a connection line to the single telemetry server.
   *
   * The target URI and OnConnectedCallback are to be passed to the
   * implementation class' constructor.
   */
  class TelemetryConnection {
   public:
    /**
     * The callback to be called each time the connection establishes.
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

    /// Initiates attempts to connect
    virtual void connect() = 0;

    /// Returns the associated telemetry endpoint
    virtual const TelemetryEndpoint &endpoint() const = 0;

    /**
     * Write the data to the websocket if connected
     * @param data - the data line to send
     *
     * the data might be disposed in an outer scope
     * as soon as the method returns
     */
    virtual void send(const std::string &data) = 0;

    /// Get the current status of the connection
    virtual bool isConnected() const = 0;

    /// Request the connection to shutdown
    virtual void shutdown() = 0;
  };

}  // namespace kagome::telemetry

#endif  // KAGOME_TELEMETRY_CONNECTION_HPP
