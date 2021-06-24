/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "metrics/impl/exposer_impl.hpp"

#include <thread>

#include "metrics/impl/session_impl.hpp"

namespace kagome::metrics {
  ExposerImpl::ExposerImpl(
      const std::shared_ptr<application::AppStateManager> &app_state_manager,
      Exposer::Configuration exposer_config,
      Session::Configuration session_config)
      : logger_{log::createLogger("OpenMetrics", "metrics")},
        context_{std::make_shared<Context>()},
        config_{std::move(exposer_config)},
        session_config_{session_config} {
    BOOST_ASSERT(app_state_manager);
    app_state_manager->takeControl(*this);
  }

  bool ExposerImpl::prepare() {
    try {
      acceptor_ = std::make_unique<Acceptor>(*context_, config_.endpoint);
    } catch (const boost::wrapexcept<boost::system::system_error> &exception) {
      SL_CRITICAL(logger_, "Failed to prepare a listener: {}", exception.what());
      return false;
    } catch (const std::exception &exception) {
      SL_CRITICAL(logger_, "Exception when preparing a listener: {}",
                        exception.what());
      return false;
    }

    boost::system::error_code ec;
    acceptor_->set_option(boost::asio::socket_base::reuse_address(true), ec);
    if (ec) {
      logger_->error("Failed to set `reuse address` option to acceptor");
      return false;
    }
    return true;
  }

  bool ExposerImpl::start() {
    BOOST_ASSERT(acceptor_);

    if (!acceptor_->is_open()) {
      logger_->error("error: trying to start on non opened acceptor");
      return false;
    }

    logger_->info("Started successfully on host: {}, port: {}",
                  config_.endpoint.address(),
                  config_.endpoint.port());
    acceptOnce();

    thread_ = std::make_shared<std::thread>(
        [context = context_]() { context->run(); });
    thread_->detach();

    return true;
  }

  void ExposerImpl::stop() {
    if (acceptor_) {
      acceptor_->cancel();
    }
    context_->stop();
  }

  void ExposerImpl::acceptOnce() {
    new_session_ = std::make_shared<SessionImpl>(*context_, session_config_);
    new_session_->connectOnRequest(std::bind(&Handler::onSessionRequest,
                                             handler_.get(),
                                             std::placeholders::_1,
                                             std::placeholders::_2));

    auto on_accept = [wp = weak_from_this()](boost::system::error_code ec) {
      if (auto self = wp.lock()) {
        if (not ec) {
          self->new_session_->start();
        }

        if (self->acceptor_->is_open()) {
          // continue to accept until acceptor is ready
          self->acceptOnce();
        }
      }
    };

    acceptor_->async_accept(new_session_->socket(), std::move(on_accept));
  }
}  // namespace kagome::metrics
