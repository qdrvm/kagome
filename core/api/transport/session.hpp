/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_TRANSPORT_SESSION_HPP
#define KAGOME_CORE_API_TRANSPORT_SESSION_HPP

#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <boost/signals2/signal.hpp>

#include "api/transport/rpc_io_context.hpp"

namespace kagome::api {
  enum struct SessionType {
    kHttp = 1,
    kWs,
  };

  /**
   * @brief rpc session
   */
  class Session {
    template <class T>
    using Signal = boost::signals2::signal<T>;

    using OnRequestSignature = void(std::string_view,
                                    std::shared_ptr<Session> session);
    using OnRequest = Signal<OnRequestSignature>;

   public:
    using Context = RpcContext;
    using Socket = boost::asio::ip::tcp::socket;
    using ErrorCode = boost::system::error_code;
    using Streambuf = boost::asio::streambuf;
    using Timer = boost::asio::steady_timer;
    using Connection = boost::signals2::connection;
    using Duration = Timer::duration;
    using SessionId = uint64_t;

    /// Event handlers.
    using OnCloseHandler = std::function<void(SessionId, SessionType)>;

    virtual ~Session() = default;

    /**
     * @brief connects `on request` callback
     * @param callback `on request` callback
     */
    void connectOnRequest(std::function<OnRequestSignature> callback) {
      on_request_ = std::move(callback);
    }

    /**
     * @brief connects `on close` callback
     * @param handler `on close` event handler
     */
    void connectOnCloseHandler(OnCloseHandler &&handler) {
      on_close_ = std::move(handler);
    }

    /**
     * @brief process request message
     * @param request message to process
     */
    void processRequest(std::string_view request,
                        std::shared_ptr<Session> session) {
      on_request_(request, std::move(session));
    }

    /**
     * @brief send response message
     * @param message response message
     */
    virtual void respond(std::string_view message) = 0;

    /**
     * @brief makes `on close` notification to listener
     * @param id session id
     * @param type type of the closed session
     */
    void notifyOnClose(SessionId id, SessionType type) {
      if (nullptr != on_close_) {
        on_close_(id, type);
      }
    }

    /**
     * @brief method to get id of the session
     * @return id of the session
     */
    virtual SessionId id() const = 0;

    /**
     * @brief method to get type of the session
     * @return type of the session
     */
    virtual SessionType type() const = 0;

    // TODO(turuslan): #1474, refactor jrpc notifications
    /**
     * `boost::asio::post(io_context, cb)` to `io_context` of this session.
     *
     * Callback is pushed to write queue and will be called after all currently
     * queued writes will complete.
     */
    virtual void post(std::function<void()> cb) = 0;

    /**
     * Can call unsafe methods
     */
    virtual bool isUnsafeAllowed() const = 0;

   private:
    std::function<OnRequestSignature> on_request_;  ///< `on request` callback
    OnCloseHandler on_close_;
  };

}  // namespace kagome::api

#endif  // KAGOME_CORE_API_TRANSPORT_SESSION_HPP
