/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/transport/impl/session_impl.hpp"

namespace kagome::server {
  SessionImpl::SessionImpl(Session::Socket socket, Session::Context &context)
      : socket_(std::move(socket)), heartbeat_(context) {
    heartbeat_.async_wait(std::bind(&SessionImpl::processHeartBeat, this));
    onResponse().connect(
        [this](std::string response) { processResponse(std::move(response)); });
  }

  void SessionImpl::start() {
    asyncRead();
  }

  void SessionImpl::stop() {
    heartbeat_.cancel();
    socket_.cancel();
    onStopped()(shared_from_this());
  }

  void SessionImpl::asyncRead() {
    boost::asio::async_read_until(
        socket_, buffer_, '\n',
        [this, self = shared_from_this()](ErrorCode ec, std::size_t length) {
          heartbeat_.cancel();
          if (!ec) {
            std::string data((std::istreambuf_iterator<char>(&buffer_)),
                             std::istreambuf_iterator<char>());
            onRequest()(shared_from_this(), std::move(data));
          } else {
            stop();
          }
        });
  }

  void SessionImpl::processResponse(std::string response) {
    // 10 seconds wait for new request, then close session
    heartbeat_.expires_after(std::chrono::seconds(10));
    asyncWrite(std::move(response));
  }

  void SessionImpl::asyncWrite(std::string response) {
    auto r = std::make_shared<std::string>(std::move(response));

    boost::asio::async_write(socket_,
                             boost::asio::const_buffer(r->data(), r->size()),
                             [this, self = shared_from_this(), r](
                                 boost::system::error_code ec, std::size_t
                                 /*length*/) {
                               if (!ec) {
                                 asyncRead();
                               } else {
                                 stop();
                               }
                             });
  }

  void SessionImpl::processHeartBeat() {
    // TODO(yuraz): pre-230 addD log message
    stop();
  }
}  // namespace kagome::server
