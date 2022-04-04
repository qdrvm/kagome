/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TELEMETRY_CONNECTION_IMPL_HPP
#define KAGOME_TELEMETRY_CONNECTION_IMPL_HPP

#include "telemetry/connection.hpp"

#include <chrono>
#include <memory>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/variant.hpp>
#include "log/logger.hpp"

namespace kagome::telemetry {

  /**
   * Sets operations' timeout during websocket connection establishing
   */
  static constexpr auto kConnectionTimeout = std::chrono::seconds(30);

  class TelemetryConnectionImpl
      : public TelemetryConnection,
        public std::enable_shared_from_this<TelemetryConnectionImpl> {
   public:
    TelemetryConnectionImpl(std::shared_ptr<boost::asio::io_context> io_context,
                            const TelemetryEndpoint &endpoint,
                            OnConnectedCallback callback);
    TelemetryConnectionImpl(const TelemetryConnectionImpl &) = delete;
    TelemetryConnectionImpl(TelemetryConnectionImpl &&) = delete;

    void connect() override;

    const TelemetryEndpoint &endpoint() const override;

    void send(const std::string &data) override;

    bool isConnected() const override;

    void shutdown() override;

   private:
    std::shared_ptr<boost::asio::io_context> io_context_;
    const TelemetryEndpoint endpoint_;
    OnConnectedCallback callback_;
    bool is_connected_ = false;
    bool shutdown_requested_ = false;
    log::Logger log_;

    uint16_t port_ = 80;
    bool secure_ = false;
    std::string path_;
    std::string ws_handshake_hostname_;

    boost::asio::ssl::context ssl_ctx_;
    boost::asio::ip::tcp::resolver resolver_;

    using TcpStream = boost::beast::tcp_stream;
    using SslStream = boost::beast::ssl_stream<TcpStream>;
    template <typename T>
    using WsStream = boost::beast::websocket::stream<T>;
    using WsTcpStream = WsStream<TcpStream>;
    using WsSslStream = WsStream<SslStream>;
    using WsTcpStreamPtr = std::unique_ptr<WsTcpStream>;
    using WsSslStreamPtr = std::unique_ptr<WsSslStream>;
    boost::variant<WsTcpStreamPtr, WsSslStreamPtr> ws_;

    static size_t instance_;

    boost::beast::lowest_layer_type<SslStream> &stream_lowest_layer();

    template <typename WsStreamT>
    void write(WsStreamT &ws, const std::string &data);

    void onResolve(boost::beast::error_code ec,
                   boost::asio::ip::tcp::resolver::results_type results);

    void onConnect(
        boost::beast::error_code ec,
        boost::asio::ip::tcp::resolver::results_type::endpoint_type endpoint);

    template <typename WsStreamT>
    void setOptionsAndRunWsHandshake(WsStreamT &ws);

    void onSslHandshake(boost::beast::error_code ec);

    void onHandshake(boost::beast::error_code ec);

    void close();
  };

}  // namespace kagome::telemetry

#endif  // KAGOME_TELEMETRY_CONNECTION_IMPL_HPP
