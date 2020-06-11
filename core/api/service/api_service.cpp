/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/api_service.hpp"

#include "api/jrpc/jrpc_processor.hpp"

namespace kagome::api {

  ApiService::ApiService(
      const std::shared_ptr<application::AppStateManager> &app_state_manager,
      std::shared_ptr<api::RpcThreadPool> thread_pool,
      std::vector<std::shared_ptr<Listener>> listeners,
      std::shared_ptr<JRpcServer> server,
      gsl::span<std::shared_ptr<JRpcProcessor>> processors)
      : thread_pool_(std::move(thread_pool)),
        listeners_(std::move(listeners)),
        server_(std::move(server)),
        logger_{common::createLogger("Api service")} {
    BOOST_ASSERT(thread_pool_);
    for ([[maybe_unused]] const auto &listener : listeners_) {
      BOOST_ASSERT(listener != nullptr);
    }
    for (auto &processor : processors) {
      BOOST_ASSERT(processor != nullptr);
      processor->registerHandlers();
    }

    BOOST_ASSERT(app_state_manager);
    app_state_manager->takeControl(*this);
  }

  void ApiService::prepare() {
    for (const auto &listener : listeners_) {
      auto on_new_session =
          [wp = weak_from_this()](const sptr<Session> &session) mutable {
            if (auto self = wp.lock(); not self) {
              return;
            }
            session->connectOnRequest(
                [wp](std::string_view request,
                     std::shared_ptr<Session> session) mutable {
                  auto self = wp.lock();
                  if (not self) {
                    return;
                  }
                  // process new request
                  self->server_->processData(
                      std::string(request),
                      [session = std::move(session)](
                          const std::string &response) mutable {
                        // process response
                        session->respond(response);
                      });
                });
          };

      listener->setHandlerForNewSession(std::move(on_new_session));
    }
  }

  void ApiService::start() {
    thread_pool_->start();
    logger_->debug("Service started");
  }

  void ApiService::stop() {
    thread_pool_->stop();
    logger_->debug("Service stopped");
  }

}  // namespace kagome::api
