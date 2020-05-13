/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/transport/impl/ws/ws_listener_impl.hpp"

#include <boost/asio.hpp>

#include "api/transport/error.hpp"
#include "api/transport/impl/ws/ws_session.hpp"

namespace kagome::api {
  WsListenerImpl::WsListenerImpl(std::shared_ptr<Context> context,
                                 const Configuration &configuration,
                                 SessionImpl::Configuration session_config)
      : context_(std::move(context)),
        acceptor_(*context_, configuration.endpoint),
        session_config_{session_config} {}

  void WsListenerImpl::acceptOnce(Listener::NewSessionHandler on_new_session) {
    new_session_ = std::make_shared<SessionImpl>(*context_, session_config_);

    acceptor_.async_accept(
        new_session_->socket(),
        [wp = weak_from_this(), on_new_session = std::move(on_new_session)](
            boost::system::error_code ec) mutable {
          if (auto self = wp.lock()) {
            if (ec) {
              self->logger_->error("error: failed to start listening, code: {}",
                                   ApiTransportError::FAILED_START_LISTENING);
              self->stop();
              return;
            }

            if (self->state_ != State::WORKING) {
              self->logger_->error(
                  "error: cannot accept session, listener is in wrong state, "
                  "code: "
                  "{}",
                  ApiTransportError::CANNOT_ACCEPT_LISTENER_NOT_WORKING);

              self->stop();
              return;
            }

            on_new_session(self->new_session_);
            self->new_session_->start();

            // stay ready for new connection
            self->acceptOnce(std::move(on_new_session));
          }
        });
  }

  void WsListenerImpl::start(Listener::NewSessionHandler on_new_session) {
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
    acceptOnce(std::move(on_new_session));
  }

  void WsListenerImpl::stop() {
    state_ = State::STOPPED;
    acceptor_.cancel();
  }

}  // namespace kagome::api
