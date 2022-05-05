/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TELEMETRY_CONNECTION_IMPL_HPP
#define KAGOME_TELEMETRY_CONNECTION_IMPL_HPP

#include "telemetry/connection.hpp"

#include <chrono>
#include <memory>
#include <queue>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/variant.hpp>
#include <libp2p/basic/scheduler.hpp>
#include "log/logger.hpp"
#include "telemetry/impl/message_pool.hpp"

namespace kagome::telemetry {

  /// operations' timeout during websocket connection establishing
  static constexpr auto kConnectionTimeout = std::chrono::seconds(30);

  /// starting value for reconnection timeout in case line failure
  static constexpr auto kInitialReconnectTimeout = std::chrono::seconds(5);

  /**
   * Telemetry connection class implementation.
   */
  class TelemetryConnectionImpl
      : public TelemetryConnection,
        public std::enable_shared_from_this<TelemetryConnectionImpl> {
   public:
    /**
     * Initializes the connection instance
     * @param io_context - io_context to serve the network events
     * @param endpoint - telemetry endpoint to connect to
     * @param callback - callback to notify when connection gets established
     * @param message_pool - the pool to read messages passed by handle
     * @param scheduler - scheduler for reconnecting in case of line failure
     */
    TelemetryConnectionImpl(
        std::shared_ptr<boost::asio::io_context> io_context,
        const TelemetryEndpoint &endpoint,
        OnConnectedCallback callback,
        std::shared_ptr<MessagePool> message_pool,
        std::shared_ptr<libp2p::basic::Scheduler> scheduler);
    TelemetryConnectionImpl(const TelemetryConnectionImpl &) = delete;
    TelemetryConnectionImpl(TelemetryConnectionImpl &&) = delete;

    /// Initiate connection process
    void connect() override;

    /// To remind what was the endpoint ;)
    const TelemetryEndpoint &endpoint() const override;

    /**
     * Send the line over the connection
     * @param data - data to send, can be disposed immediately after returning
     */
    void send(const std::string &data) override;

    /**
     * Send the record from the message pool
     * @param message_handle - handle to the record to send
     */
    void send(MessageHandle message_handle) override;

    /// Reports connection status
    bool isConnected() const override;

    /// Request graceful connection shutdown
    void shutdown() override;

   private:
    using TcpStream = boost::beast::tcp_stream;
    using SslStream = boost::beast::ssl_stream<TcpStream>;
    template <typename T>
    using WsStream = boost::beast::websocket::stream<T>;
    using WsTcpStream = WsStream<TcpStream>;
    using WsSslStream = WsStream<SslStream>;
    using WsTcpStreamPtr = std::unique_ptr<WsTcpStream>;
    using WsSslStreamPtr = std::unique_ptr<WsSslStream>;

    boost::beast::lowest_layer_type<SslStream> &stream_lowest_layer();

    template <typename WsStreamT>
    void write(WsStreamT &ws, MessageHandle message_handle);

    void onResolve(boost::beast::error_code ec,
                   boost::asio::ip::tcp::resolver::results_type results);

    void onConnect(
        boost::beast::error_code ec,
        boost::asio::ip::tcp::resolver::results_type::endpoint_type endpoint);

    template <typename WsStreamT>
    void setOptionsAndRunWsHandshake(WsStreamT &ws);

    void onSslHandshake(boost::beast::error_code ec);

    void onHandshake(boost::beast::error_code ec);

    void releaseQueue();

    void close();

    void reconnect();

    std::shared_ptr<boost::asio::io_context> io_context_;
    const TelemetryEndpoint endpoint_;
    OnConnectedCallback callback_;
    std::shared_ptr<MessagePool> message_pool_;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;
    bool is_connected_ = false;
    bool shutdown_requested_ = false;
    log::Logger log_;

    std::chrono::seconds reconnect_timeout_ = kInitialReconnectTimeout;
    uint16_t port_ = 80;
    bool secure_ = false;
    std::string path_;
    std::string ws_handshake_hostname_;

    boost::asio::ssl::context ssl_ctx_;
    boost::asio::ip::tcp::resolver resolver_;
    boost::variant<WsTcpStreamPtr, WsSslStreamPtr> ws_;

    static std::size_t instance_;
    volatile bool busy_ = false;
    boost::circular_buffer<MessageHandle> queue_;
  };

}  // namespace kagome::telemetry

#endif  // KAGOME_TELEMETRY_CONNECTION_IMPL_HPP
