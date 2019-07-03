/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/transport/impl/listener_impl.hpp"

namespace kagome::server {

  ListenerImpl::ListenerImpl(boost::asio::io_context &context,
                             boost::asio::ip::tcp::endpoint endpoint)
      : context_(context),
        acceptor_(context_, std::move(endpoint)),
        session_manager_(context_) {
    onError().connect([this](outcome::result<void> err) {
      // TODO(yuraz): pre-230 log error
      stop();
    });
  }

  void ListenerImpl::doAccept() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, Session::Socket socket) {
          if (!ec) {
            if (state_ != State::WORKING) {
              // TODO(yuraz): PRE-230 add log
              return;
            }
            auto id = session_manager_.newSession(std::move(socket));
            auto session = session_manager_[id];
            onNewSession()(session);
          } else {
            onError()(outcome::failure(ec));
          }
        });
  }

  void ListenerImpl::start() {
    state_ = State::WORKING;
    doAccept();
  }

  void ListenerImpl::stop() {
    state_ = State::STOPPED;
    acceptor_.cancel();
    onStopped();
  }
}  // namespace kagome::server
