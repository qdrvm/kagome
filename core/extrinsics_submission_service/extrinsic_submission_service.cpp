/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extrinsics_submission_service/extrinsic_submission_service.hpp"

namespace kagome::service {
  ExtrinsicSubmissionService::ExtrinsicSubmissionService(
      Configuration configuration, sptr<JsonTransport> transport,
      sptr<ExtrinsicSubmissionApi> api)
      : configuration_{configuration},
        transport_{std::move(transport)},
        api_proxy_(std::make_shared<ExtrinsicSubmissionProxy>(std::move(api))) {
    request_cnn_ = transport_->dataReceived().connect(
        [this](std::string_view data) { processData(data); });

    response_cnn_ = on_response_.connect(transport_->onResponse());

    // register json format handler
    server_.RegisterFormatHandler(format_handler_);

    auto &dispatcher = server_.GetDispatcher();

    // register all api methods
    dispatcher.AddMethod("author_submitExtrinsic",
                         &ExtrinsicSubmissionProxy::submit_extrinsic,
                         *api_proxy_);

    //    dispatcher.AddMethod("author_pendingExtrinsics",
    //                         &ExtrinsicSubmissionProxy::pending_extrinsics,
    //                         *api_proxy_);
    // other methods to be registered as soon as implemented
  }

  void ExtrinsicSubmissionService::processData(std::string_view data) {
    auto &&formatted_response = server_.HandleRequest(std::string(data));
    std::string response(formatted_response->GetData(),
                         formatted_response->GetSize());

    on_response_(response);
  }

  outcome::result<void> ExtrinsicSubmissionService::start() {
    return transport_->start();
  }

  void ExtrinsicSubmissionService::stop() {
    transport_->stop();
  }
}  // namespace kagome::service
