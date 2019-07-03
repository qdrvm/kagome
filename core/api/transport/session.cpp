/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/transport/session.hpp"

#include "api/transport/session_manager.hpp"

namespace kagome::server {

  Session::Session(Session::Socket socket, Session::Id id,
                   Session::io_context &context, SessionManager &manager)
      : id_{id}, socket_(std::move(socket)), heartbeat_(context) {
    heartbeat_.async_wait(std::bind(&Session::processHeartBeat, this));
    on_stopped_cnn_ = manager.subscribeOnClosed(on_stopped_);
  }

  Session::~Session() {
    stop();
  }

  void Session::start() {
    do_read();
  }

  void Session::stop() {
    heartbeat_.cancel();
    socket_.cancel();
    state_ = State::STOPPED;
    if (!on_stopped_.empty()) {
      on_stopped_(id());
    }
  }

  void Session::do_read() {
    boost::asio::async_read_until(
        socket_, buffer_, '\n',
        [this, self = shared_from_this()](error_code ec, std::size_t length) {
          heartbeat_.cancel();
          if (!ec) {
            std::string data((std::istreambuf_iterator<char>(&buffer_)),
                             std::istreambuf_iterator<char>());
            on_request_(shared_from_this(), data);
          } else {
            stop();
          }
        });
  }

  void Session::processResponse(std::string response) {
    // 10 seconds wait for new request, then close session
    heartbeat_.expires_after(std::chrono::seconds(10));
    do_write(response);
  }

  void Session::do_write(const std::string &response) {
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

  void Session::processHeartBeat() {
    stop();
  }
}  // namespace kagome::server
