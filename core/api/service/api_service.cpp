/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/api_service.hpp"

#include "api/jrpc/jrpc_processor.hpp"

namespace kagome::api {

  ApiService::ApiService(
      std::shared_ptr<Listener> listener,
      std::shared_ptr<JRpcServer> server,
      gsl::span<std::shared_ptr<JRpcProcessor>> processors)
      : listener_(std::move(listener)),
        server_(std::move(server)),
        logger_{common::createLogger("Api service")} {
    BOOST_ASSERT(listener_ != nullptr);
    for(auto& processor: processors) {
      BOOST_ASSERT(processor != nullptr);
      processor->registerHandlers();
    }
  }

  void ApiService::start() {
    // handle new session
    listener_->start([self = shared_from_this()](
                         const sptr<Session> &session) mutable {
      session->connectOnRequest([self](
                                    std::string_view request,
                                    std::shared_ptr<Session> session) mutable {
        // process new request
        self->server_->processData(std::string(request),
                                      [session = std::move(session)](
                                          const std::string &response) mutable {
                                        // process response
                                        session->respond(response);
                                      });
      });
    });
    logger_->debug("Service started");
  }

  void ApiService::stop() {
    listener_->stop();
  }

}  // namespace kagome::api
