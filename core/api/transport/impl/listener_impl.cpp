/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/transport/impl/listener_impl.hpp"

#include <boost/asio.hpp>
#include "api/transport/error.hpp"
#include "api/transport/impl/http_session.hpp"

namespace kagome::api {
  ListenerImpl::ListenerImpl(ListenerImpl::Context &context,
                             const ListenerImpl::Endpoint &endpoint,
                             HttpSession::Configuration http_config)
      : context_(context),
        acceptor_(boost::asio::make_strand(context_), endpoint),
        http_config_{http_config} {
    onError().connect([](auto &&...) {
      // TODO(yuraz): pre-249 log error
    });
  }

  void ListenerImpl::acceptOnce(Listener::NewSessionHandler on_new_session) {
    acceptor_.async_accept([self = shared_from_this(), on_new_session](
                               boost::system::error_code ec,
                               Session::Socket socket) mutable {
      if (ec) {
        self->onError()(ApiTransportError::FAILED_START_LISTENING);
        self->stop();
        return;
      }

      if (self->state_ != State::WORKING) {
        // TODO(yuraz): PRE-249 add log
        self->onError()(ApiTransportError::CANNOT_ACCEPT_LISTENER_NOT_WORKING);
        self->stop();
        return;
      }

      auto session =
          std::make_shared<HttpSession>(std::move(socket), self->http_config_);

      on_new_session(session);
      session->start();

      // stay ready for new connection
      self->acceptOnce(std::move(on_new_session));
    });
  }

  void ListenerImpl::start(Listener::NewSessionHandler on_new_session) {
    if (state_ == State::WORKING) {
      onError()(ApiTransportError::LISTENER_ALREADY_STARTED);
      return;
    }
    // Allow address reuse
    boost::system::error_code ec;
    acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
    if (ec) {
      onError()(ApiTransportError::FAILED_SET_OPTION);
      stop();
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
