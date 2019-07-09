/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/transport/impl/session_impl.hpp"

namespace kagome::server {
  SessionImpl::SessionImpl(Session::Socket socket, Session::Context &context,
                           Duration timeout)
      : socket_(std::move(socket)), heartbeat_(context), timeout_{timeout} {
    resetTimer();
    onResponse().connect(
        [this](std::string response) { processResponse(std::move(response)); });
  }

  void SessionImpl::start() {
    asyncRead();
  }

  void SessionImpl::stop() {
    stopTimer();
    socket_.cancel();
    onStopped()(shared_from_this());
  }

  void SessionImpl::asyncRead() {
    boost::asio::async_read_until(
        socket_, buffer_, '\n',
        [self = shared_from_this()](ErrorCode ec, std::size_t length) {
          self->stopTimer();
          if (!ec) {
            std::string data((std::istreambuf_iterator<char>(&self->buffer_)),
                             std::istreambuf_iterator<char>());
            self->onRequest()(self, std::move(data));
          } else {
            self->stop();
          }
        });
  }

  void SessionImpl::processResponse(std::string response) {
    resetTimer();
    asyncWrite(std::move(response));
  }

  void SessionImpl::asyncWrite(std::string response) {
    auto r = std::make_shared<std::string>();
    *r = std::move(response);

    boost::asio::async_write(
        socket_, boost::asio::const_buffer(r->data(), r->size()),
        [self = shared_from_this(), holder = std::move(r)](
            boost::system::error_code ec, std::size_t) {
          if (!ec) {
            self->asyncRead();
          } else {
            self->stop();
          }
        });
  }

  void SessionImpl::processHeartBeat() {
    if (heartbeat_.expiry() <= boost::asio::steady_timer::clock_type::now()) {
      stop();
    }
  }

  void SessionImpl::resetTimer() {
    heartbeat_.expires_after(timeout_);
    heartbeat_.async_wait(std::bind(&SessionImpl::processHeartBeat, this));
  }

  void SessionImpl::stopTimer() {
    heartbeat_.cancel();
  }
}  // namespace kagome::server
