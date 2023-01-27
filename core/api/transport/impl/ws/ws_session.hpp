/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_TRANSPORT_IMPL_WS_SESSION_HPP
#define KAGOME_CORE_API_TRANSPORT_IMPL_WS_SESSION_HPP

#include <chrono>
#include <cstdlib>
#include <memory>
#include <queue>

#include <boost/asio/strand.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/websocket.hpp>

#include "api/transport/session.hpp"
#include "log/logger.hpp"

namespace kagome::api {

  class WsSession : public Session,
                    public std::enable_shared_from_this<WsSession> {
    using WsError = boost::beast::websocket::error;
    using OnWsSessionCloseHandler = std::function<void()>;

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
     * @param id session id
     */
    WsSession(Context &context, Configuration config, SessionId id);

    Socket &socket() override {
      return socket_;
    }

    /**
     * @brief starts session
     */
    void start() override;

    /**
     * @brief method to get id of the session
     * @return id of the session
     */
    Session::SessionId id() const override;

    /**
     * @brief method to get type of the session
     * @return type of the session
     */
    SessionType type() const override {
      return SessionType::kWs;
    }

    /**
     * @brief sends response wrapped by websocket frame
     * @param response message to send
     */
    void respond(std::string_view response) override;

    void post(std::function<void()> cb) override;

    /**
     * @brief Closes the incoming connection with "try again later" response
     */
    void reject();

    /**
     * @brief connects `on websocket close` callback.
     * Used to maintain the maximum number of simultaneous sessions
     * @param handler `on close` event handler
     */
    void connectOnWsSessionCloseHandler(OnWsSessionCloseHandler &&handler);

   private:
    /**
     * @brief stops session
     */
    void stop();

    /**
     * @brief stops session specifying the reason
     * @param code for close reason
     */
    void stop(boost::beast::websocket::close_code code);

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
    boost::beast::websocket::stream<boost::asio::ip::tcp::socket &> stream_;
    boost::beast::flat_buffer rbuffer_;  ///< read buffer

    std::queue<std::string> pending_responses_;

    bool writing_in_progress_ = false;
    std::atomic_bool stopped_ = false;

    SessionId const id_;
    OnWsSessionCloseHandler on_ws_close_;
    log::Logger logger_ = log::createLogger("WsSession", "rpc_transport");
  };

}  // namespace kagome::api

#endif  // KAGOME_CORE_API_TRANSPORT_BEAST_HTTP_SESSION_HPP
