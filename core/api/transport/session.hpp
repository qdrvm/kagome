/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_TRANSPORT_SESSION_HPP
#define KAGOME_CORE_API_TRANSPORT_SESSION_HPP

#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <boost/signals2/signal.hpp>
#include "api/transport/basic_transport.hpp"

namespace kagome::server {
  /**
   * @brief keeps and manages sessions
   */
  class SessionManager;
  /**
   * @brief rpc session
   */
  class Session : public api::BasicTransport,
                  public std::enable_shared_from_this<Session> {
    /**
     * @brief session state
     */
    enum class State {
      READY,
      WAITING_FOR_REQUEST,
      PROCESSING_RESPONSE,
      STOPPED
    };

   public:
    using Socket = boost::asio::ip::tcp::socket;
    using io_context = boost::asio::io_context;
    using error_code = boost::system::error_code;
    using Streambuf = boost::asio::streambuf;
    using Timer = boost::asio::steady_timer;
    using Id = uint64_t;
    using Connection = boost::signals2::connection;
    template <class T>
    using Signal = boost::signals2::signal<T>;

    Session(Socket socket, Id id, io_context &context, SessionManager &manager);

    ~Session() override;

    /**
     * @return session id
     */
    inline Id id() const {
      return id_;
    }

    /**
     * @brief starts listening on socket
     */
    void start() override;

    /**
     * @brief closes connection and session
     */
    void stop() override;

    /**
     * @brief connects to data processor
     * @param worker data processor
     */
    void connect(server::WorkerApi &worker) override;

   private:
    /**
     * starts listening to requests
     */
    void do_read();

    /**
     * @brief runs when responce is obtained
     * @param response json string response
     */
    void processResponse(const std::string &response) override;

    /**
     * @brief sends response
     * @param response json result string
     */
    void do_write(const std::string &response);

    /**
     * @brief is called when no requests for too long time
     * closes session
     */
    void processHeartBeat();

    Id id_;                                 ///< session id
    State state_{State::READY};             ///< session state
    Socket socket_;                         ///< socket
    Timer heartbeat_;                       ///< heartbeat timer
    Streambuf buffer_;                      ///< request buffer
    Signal<void(Session::Id)> on_stopped_;  ///< signal to be sent to manager
                                            ///< for processing `stopped` event

    Signal<void(Session::Id, const std::string &)> on_request_;
    Signal<void(const std::string &)> on_response_;

    Connection on_stopped_cnn_;   ///< connection for on_stopped event
    Connection on_request_cnn_;   ///< connetion for `on request` event
    Connection on_response_cnn_;  ///< connection for `on response` event
  };

}  // namespace kagome::server

#endif  // KAGOME_CORE_API_TRANSPORT_SESSION_HPP
