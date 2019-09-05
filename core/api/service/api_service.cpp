/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/api_service.hpp"
#include "api/jrpc/jrpc_processor.hpp"

namespace kagome::api {

  ApiService::ApiService(std::shared_ptr<Listener> listener,
                         std::shared_ptr<JRPCProcessor> processor)
      : listener_(std::move(listener)), processor_(std::move(processor)) {
    listener_->onError().connect([this](outcome::result<void> err) { stop(); });
  }

  void ApiService::start() {
    // handle new session
    listener_->start([this](sptr<Session> session) mutable {
      session->onRequest().connect(
          [this, session](std::string_view request) mutable {
            // process new request
            processor_->processData(std::string(request),
                                    [session = std::move(session)](
                                        const std::string &response) mutable {
                                      // process response
                                      session->onResponse()(response);
                                    });
          });
    });
  }

  void ApiService::stop() {
    listener_->stop();
  }

}  // namespace kagome::api
