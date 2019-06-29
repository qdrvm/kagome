/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_TRANSPORT_SERVER_HPP
#define KAGOME_CORE_API_TRANSPORT_SERVER_HPP

#include <memory>
#include <unordered_map>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "api/transport/session.hpp"

namespace kagome::server {
  /**
   * @brief server accepts connections making session from socket
   */
  class Server {
    /**
     * @brief server state
     */
    enum class State { READY, WORKING, STOPPED = READY };
    template <class T>
    using sptr = std::shared_ptr<Session>;

    using io_context = boost::asio::io_context;
    using Acceptor = boost::asio::ip::tcp::acceptor;

   public:
    using Endpoint = boost::asio::ip::tcp::endpoint;

    Server(io_context &context, Endpoint endpoint)
        : context_(context), acceptor_(context_, std::move(endpoint)) {}

    void start() {
      state_ = State::WORKING;
      doAccept();
    }

    void stop() {
      state_ = State::STOPPED;
      acceptor_.cancel();
    }

   private:
    Session::Id nextSessionId() {
      return lastSessionid_++;
    }

    bool registerSession(sptr<Session> session) {
      if (sessions_.find(session->id()) == std::end(sessions_)) {
        sessions_.insert({session->id(), std::move(session)});
        return true;
      }
      return false;
    }

    void doAccept() {
      acceptor_.async_accept(
          [this](boost::system::error_code ec, Session::Socket socket) {
            if (!ec) {
              auto session = std::make_shared<Session>(
                  std::move(socket), nextSessionId(), context_);
              session->start();
              registerSession(session);
            }

            if (state_ == State::WORKING) {
              doAccept();
            }
          });
    }

    io_context &context_;
    Acceptor acceptor_;

    State state_{State::READY};
    Session::Id lastSessionid_{0u};
    std::unordered_map<Session::Id, sptr<Session>> sessions_;
  };
}  // namespace kagome::server

#endif  // KAGOME_CORE_API_TRANSPORT_SERVER_HPP
