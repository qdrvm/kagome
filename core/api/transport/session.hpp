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
#include "api/transport/basic_transport.hpp"

namespace kagome::server {
  /**
   * @brief rpc session
   */
  class Session : public std::enable_shared_from_this<Session>,
                  public api::BasicTransport {
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

    Session(Socket socket, Id id, io_context &context)
        : socket_(std::move(socket)), deadline_(context), id_{id} {
      deadline_.async_wait(std::bind(&Session::checkDeadline, this));
    }

    Id id() const {
      return id_;
    }

    void start() override {
      do_read();
    }

    void stop() override {
      deadline_.cancel();
    }

   private:
    void do_read() {
      boost::asio::async_read_until(
          socket_, buffer_, '\n',
          [this, self = shared_from_this()](error_code ec, std::size_t length) {
            deadline_.cancel();
            if (!ec) {
              std::string data(buffer_.data().begin(), buffer_.data().end());
              dataReceived()(data);
            } else {
              stop();
            }
          });
    }

    /**
     * @brief runs when responce is obtained
     * @param response json string response
     */
    void processResponse(const std::string &response) override {
      // 10 seconds for new request
      deadline_.expires_at(std::chrono::seconds(10));
      do_write(response);
    }

    void do_write(const std::string &response) {
      boost::asio::async_write(
          socket_, boost::asio::buffer(response.data(), response.size()),
          [this, self = shared_from_this()](
              boost::system::error_code ec, std::size_t
              /*length*/) {
            if (!ec) {
              do_read();
            } else {
              stop();
            }
          });
    }

   private:
    void checkDeadline() {}

    State state_{State::READY};
    Socket socket_;
    Timer deadline_;
    Streambuf buffer_;

    Id id_;  ///< session id
  };

}  // namespace kagome::server

#endif  // KAGOME_CORE_API_TRANSPORT_SESSION_HPP
