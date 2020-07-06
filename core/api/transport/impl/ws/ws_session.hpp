/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_TRANSPORT_IMPL_WS_SESSION_HPP
#define KAGOME_CORE_API_TRANSPORT_IMPL_WS_SESSION_HPP

#include <boost/asio/strand.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/websocket.hpp>
#include <chrono>
#include <cstdlib>
#include <memory>

#include "api/transport/session.hpp"
#include "common/logger.hpp"

namespace kagome::api {

  class WsSession : public Session,
                    public std::enable_shared_from_this<WsSession> {
    using WsError = boost::beast::websocket::error;

   public:
    struct Configuration {
      static constexpr size_t kDefaultRequestSize = 10000u;
      static constexpr Duration kDefaultTimeout = std::chrono::seconds(30);

      size_t max_request_size{kDefaultRequestSize};
      Duration operation_timeout{kDefaultTimeout};
    };

    ~WsSession() override = default;

    /**
     * @brief constructor
     * @param socket socket instance
     * @param config session configuration
     */
    WsSession(Context &context, Configuration config);

    Socket &socket() override {
      return socket_;
    }

    /**
     * @brief starts session
     */
    void start() override;

    /**
     * @brief sends response wrapped by websocket frame
     * @param response message to send
     */
    void respond(std::string_view response) override;

   private:
    /**
     * @brief stops session
     */
    void stop();

    /**
     * @brief process received websocket frame, compose and execute response
     * @tparam Body request body type
     * @param request request
     */
    void handleRequest(std::string_view data);

    /**
     * @brief asynchronously read
     */
    void asyncRead();

    /**
     * @brief asynchronously write
     */
    void asyncWrite();

    /**
     * @brief connected callback
     */
    void onRun();

    /**
     * @brief handshake completion callback
     */
    void onAccept(boost::system::error_code ec);

    /**
     * @brief read completion callback
     */
    void onRead(boost::system::error_code ec, std::size_t size);

    /**
     * @brief write completion callback
     */
    void onWrite(boost::system::error_code ec, std::size_t bytes_transferred);

    /**
     * @brief reports error code and message
     * @param ec error code
     * @param message error message
     */
    void reportError(boost::system::error_code ec, std::string_view message);

    /// Strand to ensure the connection's handlers are not called concurrently.
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;

    /// Socket for the connection.
    boost::asio::ip::tcp::socket socket_;

    Configuration config_;  ///< session configuration
    boost::beast::websocket::stream<boost::asio::ip::tcp::socket &>
        stream_;                             ///< stream
    boost::beast::flat_buffer rbuffer_;  ///< read buffer
    boost::beast::flat_buffer wbuffer_;  ///< write buffer

    common::Logger logger_ =
        common::createLogger("websocket session");  ///< logger instance
  };

}  // namespace kagome::api

#endif  // KAGOME_CORE_API_TRANSPORT_BEAST_HTTP_SESSION_HPP
