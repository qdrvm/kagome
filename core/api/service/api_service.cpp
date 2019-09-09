/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/api_service.hpp"

#include "api/jrpc/jrpc_processor.hpp"

namespace kagome::api {

  ApiService::ApiService(std::shared_ptr<Listener> listener,
                         std::shared_ptr<JRPCProcessor> processor)
      : listener_(std::move(listener)), processor_(std::move(processor)) {}

  void ApiService::start() {
    // handle new session
    listener_->start([self = shared_from_this()](
                         const sptr<Session> &session) mutable {
      session->connectOnRequest([self](
                                    std::string_view request,
                                    std::shared_ptr<Session> session) mutable {
        // process new request
        self->processor_->processData(std::string(request),
                                      [session = std::move(session)](
                                          const std::string &response) mutable {
                                        // process response
                                        session->respond(response);
                                      });
      });
    });
  }

  void ApiService::stop() {
    listener_->stop();
  }

}  // namespace kagome::api
