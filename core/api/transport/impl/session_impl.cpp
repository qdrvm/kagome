/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/transport/impl/session_impl.hpp"

namespace kagome::api {
  SessionImpl::SessionImpl(Session::Socket socket,
                           Session::Context &context,
                           Duration timeout)
      : socket_(std::move(socket)), heartbeat_(context), timeout_{timeout} {
    resetTimer();
    connectOnResponse(
        [this](std::string_view response) { processResponse(response); });
  }

  void SessionImpl::start() {
    asyncRead();
  }

  void SessionImpl::stop() {
    stopTimer();
    socket_.cancel();
  }

  void SessionImpl::asyncRead() {
    boost::asio::async_read_until(
        socket_,
        buffer_,
        '\n',
        [self = shared_from_this()](ErrorCode ec, std::size_t length) {
          self->stopTimer();
          if (ec) {
            return self->stop();
          }
          std::string data((std::istreambuf_iterator<char>(&self->buffer_)),
                           std::istreambuf_iterator<char>());
          self->processRequest(data);
        });
  }

  void SessionImpl::processResponse(std::string_view response) {
    resetTimer();
    asyncWrite(response);
  }

  void SessionImpl::asyncWrite(std::string_view response) {
    auto r = std::make_shared<std::string>(std::string(response) + '\n');

    boost::asio::async_write(socket_,
                             boost::asio::const_buffer(r->data(), r->size()),
                             [self = shared_from_this(), r](
                                 boost::system::error_code ec, std::size_t) {
                               if (ec) {
                                 return self->stop();
                               }
                               self->asyncRead();
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
}  // namespace kagome::api
