/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_TRANSPORT_IMPL_WS_SESSION_HPP
#define KAGOME_CORE_API_TRANSPORT_IMPL_WS_SESSION_HPP

#include <chrono>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <queue>

#include <boost/asio/strand.hpp>
#include <boost/beast/core/buffered_read_stream.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/websocket.hpp>

#include "api/allow_unsafe.hpp"
#include "api/transport/listener.hpp"
#include "api/transport/session.hpp"
#include "log/logger.hpp"

namespace kagome::api {
  class WsSession;

  class WsSessionImpl : public Session {
   public:
    WsSessionImpl(std::shared_ptr<WsSession> impl,
                  SessionId id,
                  SessionType type,
                  bool unsafe_allowed);

    ~WsSessionImpl() override = default;

    void respond(std::string_view response) override;

    SessionId id() const override {
      return id_;
    }

    SessionType type() const override {
      return type_;
    }

    void post(std::function<void()> cb) override;

    bool isUnsafeAllowed() const override {
      return unsafe_allowed_;
    }

    void close();

   private:
    std::mutex mutex_;
    std::shared_ptr<WsSession> impl_;
    SessionId id_;
    SessionType type_;
    bool unsafe_allowed_;
  };

  class WsSession : public std::enable_shared_from_this<WsSession> {
    using WsError = boost::beast::websocket::error;
    using OnWsSessionCloseHandler = std::function<void()>;
    using GetId = std::function<Session::SessionId()>;
    using OnSession = std::shared_ptr<Listener::NewSessionHandler>;

   public:
    struct Configuration {
      static constexpr size_t kDefaultRequestSize = 10000u;
      static constexpr Session::Duration kDefaultTimeout =
          std::chrono::seconds(30);

      size_t max_request_size{kDefaultRequestSize};
      Session::Duration operation_timeout{kDefaultTimeout};
    };

    WsSession(Session::Context &context,
              GetId get_id,
              OnSession on_session,
              AllowUnsafe allow_unsafe,
              Configuration config);

    Session::Socket &socket() {
      return stream_.next_layer().next_layer().socket();
    }

    void start();

    void respond(std::string_view response);

    void post(std::function<void()> cb);

    bool isUnsafeAllowed() const;

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
     * @brief async upgrade http to ws
     */
    void wsAccept();

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

    std::shared_ptr<WsSessionImpl> sessionMake();
    void sessionClose();

    void httpClose();
    void httpRead();
    void httpWrite();

    GetId get_id_;
    OnSession on_session_;
    AllowUnsafe allow_unsafe_;
    Configuration config_;  ///< session configuration
    boost::beast::websocket::stream<
        boost::beast::buffered_read_stream<boost::beast::tcp_stream,
                                           boost::beast::flat_buffer>>
        stream_;
    boost::beast::flat_buffer rbuffer_;  ///< read buffer
    std::optional<
        boost::beast::http::request_parser<boost::beast::http::string_body>>
        http_request_;
    std::optional<boost::beast::http::response<boost::beast::http::string_body>>
        http_response_;
    bool is_ws_ = false;

    std::queue<std::string> pending_responses_;

    bool writing_in_progress_ = false;
    std::atomic_bool stopped_ = false;

    OnWsSessionCloseHandler on_ws_close_;
    log::Logger logger_ = log::createLogger("WsSession", "rpc_transport");

    std::weak_ptr<WsSessionImpl> session_;
  };

}  // namespace kagome::api

#endif  // KAGOME_CORE_API_TRANSPORT_BEAST_HTTP_SESSION_HPP
