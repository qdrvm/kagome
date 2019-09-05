/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/transport/impl/listener_impl.hpp"

#include <boost/asio.hpp>
#include "api/transport/error.hpp"
#include "api/transport/impl/session_impl.hpp"

namespace kagome::api {
  ListenerImpl::ListenerImpl(ListenerImpl::Context &context,
                             const ListenerImpl::Endpoint &endpoint,
                             Configuration config)
      : context_(context),
        acceptor_(boost::asio::make_strand(context_), endpoint),
        config_(config) {
    onError().connect([this](auto &&...) {
      // TODO(yuraz): pre-249 log error
      stop();
    });
  }

  void ListenerImpl::acceptOnce(
      std::function<void(std::shared_ptr<Session>)> on_new_session) {
    acceptor_.async_accept([self = shared_from_this(), on_new_session](
                               boost::system::error_code ec,
                               Session::Socket socket) {
      if (ec) {
        return self->onError()(ApiTransportError::FAILED_START_LISTENING);
      }

      if (self->state_ != State::WORKING) {
        // TODO(yuraz): PRE-249 add log
        return self->onError()(
            ApiTransportError::CANNOT_ACCEPT_LISTENER_NOT_WORKING);
      }

      auto session = std::make_shared<SessionImpl>(
          std::move(socket), self->context_, self->config_.operation_timeout);
      on_new_session(session);
      session->start();
    });
  }

  void ListenerImpl::start(
      std::function<void(std::shared_ptr<Session>)> on_new_session) {
    // Allow address reuse
    boost::system::error_code ec;
    acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
    if (ec) {
      onError()(ApiTransportError::FAILED_SET_OPTION);
      return;
    }

    state_ = State::WORKING;
    acceptOnce(on_new_session);
  }

  void ListenerImpl::stop() {
    state_ = State::STOPPED;
    acceptor_.cancel();
  }
}  // namespace kagome::api
