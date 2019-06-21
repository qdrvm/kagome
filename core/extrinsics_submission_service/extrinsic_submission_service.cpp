/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extrinsics_submission_service/extrinsic_submission_service.hpp"

namespace kagome::service {
  ExtrinsicSubmissionService::ExtrinsicSubmissionService(
      Configuration configuration, sptr<JsonTransport> transport,
      sptr<ExtrinsicSubmissionApiImpl> api)
      : configuration_{configuration},
        transport_{std::move(transport)},
        api_proxy_(std::make_shared<ExtrinsicSubmissionProxy>(std::move(api))),
        on_request_([this](std::string_view data) { processData(data); }) {
    transport->dataReceived().connect(on_request_);
    on_response_.connect(transport->onResponse());

    // register json format handler
    server_.RegisterFormatHandler(json_format_handler_);

    auto &dispatcher = server_.GetDispatcher();

    // register all api methods
    dispatcher.AddMethod("author_submitExtrinsic",
                         &ExtrinsicSubmissionProxy::submit_extrinsic,
                         *api_proxy_);

    dispatcher.AddMethod("author_pendingExtrinsics",
                         &ExtrinsicSubmissionProxy::pending_extrinsics,
                         *api_proxy_);
    // other methods to be registered as soon as implemented
  }

  void ExtrinsicSubmissionService::processData(std::string_view data) {
    auto &&formatted_response = server_.HandleRequest(std::string(data));
    std::string response(formatted_response->GetData(),
                         formatted_response->GetSize());

    on_response_(response);
  }

  outcome::result<void> ExtrinsicSubmissionService::start() {
    return transport_->start(configuration_.address);
  }

  void ExtrinsicSubmissionService::stop() {
    transport_->stop();
  }
}  // namespace kagome::service
