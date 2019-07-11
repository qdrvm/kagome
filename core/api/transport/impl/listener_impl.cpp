/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/transport/impl/listener_impl.hpp"

#include <iostream>

#include "api/transport/impl/session_impl.hpp"

namespace kagome::server {
  using kagome::server::SessionImpl;

  ListenerImpl::ListenerImpl(ListenerImpl::Context &context,
                             const ListenerImpl::Endpoint &endpoint,
                             Configuration config)
      : context_(context), acceptor_(context_, endpoint), config_(config) {
    onError().connect([this](auto&&) {
      // TODO(yuraz): pre-230 log error
      stop();
    });
  }

  void ListenerImpl::doAccept() {
    acceptor_.async_accept(
        [self = shared_from_this()](boost::system::error_code ec, Session::Socket socket) {
          if (ec) {
            return self->onError()(outcome::failure(ec));
          }
          if (self->state_ != State::WORKING) {
            // TODO(yuraz): PRE-230 add log
            return;
          }
          auto session = std::make_shared<SessionImpl>(
              std::move(socket), self->context_, self->config_.operation_timeout);
          self->onNewSession()(session);
          session->start();
        });
  }

  void ListenerImpl::start() {
    state_ = State::WORKING;
    doAccept();
  }

  void ListenerImpl::stop() {
    state_ = State::STOPPED;
    acceptor_.cancel();
  }
}  // namespace kagome::server
