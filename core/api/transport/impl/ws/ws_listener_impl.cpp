/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/transport/impl/ws/ws_listener_impl.hpp"

#include <boost/asio.hpp>
#include "api/transport/tuner.hpp"
#include "application/app_state_manager.hpp"

namespace {
  constexpr auto openedRpcSessionMetricName = "kagome_rpc_sessions_opened";
  constexpr auto closedRpcSessionMetricName = "kagome_rpc_sessions_closed";
}  // namespace

namespace kagome::api {
  WsListenerImpl::WsListenerImpl(
      const std::shared_ptr<application::AppStateManager> &app_state_manager,
      std::shared_ptr<Context> context,
      Configuration listener_config,
      SessionImpl::Configuration session_config)
      : context_{std::move(context)},
        config_{std::move(listener_config)},
        session_config_{session_config},
        max_ws_connections_{config_.ws_max_connections},
        next_session_id_{1ull},
        active_connections_{0},
        log_{log::createLogger("RpcWsListener", "rpc_transport")} {
    BOOST_ASSERT(app_state_manager);

    // Register metrics
    registry_->registerCounterFamily(
        openedRpcSessionMetricName, "Number of persistent RPC sessions opened");
    opened_session_ =
        registry_->registerCounterMetric(openedRpcSessionMetricName);
    registry_->registerCounterFamily(
        closedRpcSessionMetricName, "Number of persistent RPC sessions closed");
    closed_session_ =
        registry_->registerCounterMetric(closedRpcSessionMetricName);

    app_state_manager->takeControl(*this);
  }

  bool WsListenerImpl::prepare() {
    try {
      acceptor_ = acceptOnFreePort(
          context_, config_.endpoint, kDefaultPortTolerance, log_);
    } catch (const boost::wrapexcept<boost::system::system_error> &exception) {
      SL_CRITICAL(log_, "Failed to prepare a listener: {}", exception.what());
      return false;
    } catch (const std::exception &exception) {
      SL_CRITICAL(
          log_, "Exception when preparing a listener: {}", exception.what());
      return false;
    }

    boost::system::error_code ec;
    acceptor_->set_option(boost::asio::socket_base::reuse_address(true), ec);
    if (ec) {
      SL_ERROR(log_, "Failed to set `reuse address` option to acceptor");
      return false;
    }
    return true;
  }

  bool WsListenerImpl::start() {
    BOOST_ASSERT(acceptor_);

    if (!acceptor_->is_open()) {
      SL_ERROR(log_, "An attempt to start on non-opened acceptor");
      return false;
    }
    SL_TRACE(log_, "Connections limit is set to {}", max_ws_connections_);
    SL_INFO(log_,
            "Listening for new connections on {}:{}",
            config_.endpoint.address(),
            acceptor_->local_endpoint().port());
    acceptOnce();
    return true;
  }

  void WsListenerImpl::stop() {
    if (acceptor_) {
      acceptor_->cancel();
    }
  }

  void WsListenerImpl::setHandlerForNewSession(
      NewSessionHandler &&on_new_session) {
    on_new_session_ =
        std::make_unique<NewSessionHandler>(std::move(on_new_session));
  }

  void WsListenerImpl::acceptOnce() {
    new_session_ = std::make_shared<SessionImpl>(
        *context_, session_config_, next_session_id_.fetch_add(1ull));
    auto session_stopped_handler = [wp = weak_from_this()] {
      if (auto self = wp.lock()) {
        self->closed_session_->inc();
        --self->active_connections_;
        SL_TRACE(self->log_,
                 "Session closed. Active connections count is {}",
                 self->active_connections_.load());
      }
    };

    auto on_accept = [wp = weak_from_this(),
                      session_stopped_handler](boost::system::error_code ec) {
      if (auto self = wp.lock()) {
        if (not ec) {
          self->new_session_->connectOnWsSessionCloseHandler(
              session_stopped_handler);
          if (1 + self->active_connections_.fetch_add(1)
              > self->max_ws_connections_) {
            self->new_session_->reject();
            SL_TRACE(self->log_,
                     "Connection limit ({}) reached, new connection rejected. "
                     "Active connections count is {}",
                     self->max_ws_connections_,
                     self->active_connections_.load());
          } else {
            self->opened_session_->inc();
            if (self->on_new_session_) {
              (*self->on_new_session_)(self->new_session_);
            }
            self->new_session_->start();
            SL_TRACE(self->log_,
                     "New session started. Active connections count is {}",
                     self->active_connections_.load());
          }
        }
        if (self->acceptor_->is_open()) {
          // continue to accept until acceptor is ready
          self->acceptOnce();
        }
      }
    };

    acceptor_->async_accept(new_session_->socket(), std::move(on_accept));
  }
}  // namespace kagome::api
