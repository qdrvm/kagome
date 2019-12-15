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
                             const Configuration &configuration,
                             HttpSession::Configuration http_config)
      : context_(context),
        acceptor_(context_, configuration.endpoint),
        http_config_{http_config} {}

  void ListenerImpl::acceptOnce(Listener::NewSessionHandler on_new_session) {
    acceptor_.async_accept([self = shared_from_this(), on_new_session](
                               boost::system::error_code ec,
                               Session::Socket socket) mutable {
      if (ec) {
        self->logger_->error("error: failed to start listening, code: {}",
                             ApiTransportError::FAILED_START_LISTENING);
        self->stop();
        return;
      }

      if (self->state_ != State::WORKING) {
        self->logger_->error(
            "error: cannot accept session, listener is in wrong state, code: "
            "{}",
            ApiTransportError::CANNOT_ACCEPT_LISTENER_NOT_WORKING);

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
      logger_->error(
          "error: listener already started, cannot start twice, code: {}",
          ApiTransportError::LISTENER_ALREADY_STARTED);
      return;
    }
    // Allow address reuse
    boost::system::error_code ec;
    acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
    if (ec) {
      logger_->error(
          "error: failed to set `reuse address` option to acceptor, code: {}",
          ApiTransportError::FAILED_SET_OPTION);
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
